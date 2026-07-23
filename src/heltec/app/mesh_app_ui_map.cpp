#include "mesh_app_ui.hpp"

#if defined(ENV_INCLUDE_MAP) && ENV_INCLUDE_MAP

#include "HeltecMesh.h"
#include "map_geo.hpp"
#include "ui/map/map_debug.hpp"
#include "ui/map/map_fixed_test.hpp"

#include <helpers/ContactInfo.h>

#include <math.h>
#include <string.h>

namespace heltec::meshcore::biz {
namespace {

using GpsStatus = IBizFacade::GpsStatus;

struct RawPoint {
  double lat_deg;
  double lon_deg;
  char label[32];
  bool is_self;
  int contact_index;
};

bool valid_geo_deg(double lat, double lon) {
  return isfinite(lat) && isfinite(lon) && lat >= -90.0 && lat <= 90.0 &&
         lon >= -180.0 && lon <= 180.0 && (lat != 0.0 || lon != 0.0);
}

bool usable_gps(const GpsStatus& gps) {
  return gps.fix_valid && valid_geo_deg(gps.lat_deg, gps.lon_deg);
}

void compute_map_center(const RawPoint* pts, int count, int self_idx, double& center_lat,
                        double& center_lon) {
  if (count <= 0) {
    center_lat = 0.0;
    center_lon = 0.0;
    return;
  }

  if (self_idx >= 0) {
    center_lat = pts[self_idx].lat_deg;
    center_lon = pts[self_idx].lon_deg;
  } else {
    uint32_t position_count = 0;
    double x_avg = 0.0;
    double y_avg = 0.0;
    double z_avg = 0.0;
    for (int i = 0; i < count; ++i) {
      const double lat_rad = pts[i].lat_deg * geo::kDeg2Rad;
      const double lon_rad = pts[i].lon_deg * geo::kDeg2Rad;
      const double x = cos(lat_rad) * cos(lon_rad);
      const double y = cos(lat_rad) * sin(lon_rad);
      const double z = sin(lat_rad);
      x_avg += x;
      y_avg += y;
      z_avg += z;
      position_count++;
    }
    if (position_count == 0) {
      center_lat = pts[0].lat_deg;
      center_lon = pts[0].lon_deg;
      return;
    }
    x_avg /= (double)position_count;
    y_avg /= (double)position_count;
    z_avg /= (double)position_count;
    center_lon = atan2(y_avg, x_avg) * geo::kRad2Deg;
    const double hypotenuse = sqrt(x_avg * x_avg + y_avg * y_avg);
    center_lat = atan2(z_avg, hypotenuse) * geo::kRad2Deg;
  }

  double northernmost = center_lat;
  double southernmost = center_lat;
  double easternmost = center_lon;
  double westernmost = center_lon;

  for (int i = 0; i < count; ++i) {
    const double lat_node = pts[i].lat_deg;
    northernmost = fmax(northernmost, lat_node);
    southernmost = fmin(southernmost, lat_node);

    const double lng_node = pts[i].lon_deg;
    const double deg_eastward = fmod(((lng_node - center_lon) + 360.0), 360.0);
    const double deg_westward = fabs(fmod(((lng_node - center_lon) - 360.0), 360.0));
    if (deg_eastward < deg_westward)
      easternmost = fmax(easternmost, center_lon + deg_eastward);
    else
      westernmost = fmin(westernmost, center_lon - deg_westward);
  }

  center_lat = (northernmost + southernmost) / 2.0;
  center_lon = (westernmost + easternmost) / 2.0;
  center_lon = fmod(center_lon + 540.0, 360.0) - 180.0;
}

uint32_t fnv1a_u32(uint32_t h, uint32_t v) { return (h ^ v) * 16777619u; }

uint32_t mapPlotInputFingerprint(const GpsStatus& gps) {
#if defined(HELTEC_MAP_FIXED_TEST) && HELTEC_MAP_FIXED_TEST
  if (heltec::meshcore::ui::map::mapFixedTestEnabled()) {
    return 0xF17E0001u;
  }
#endif
  uint32_t h = 2166136261u;
  const bool gps_usable = usable_gps(gps);
  h = fnv1a_u32(h, gps_usable ? 1u : 0u);
  if (gps_usable) {
    const int32_t gps_lat_micro = (int32_t)llround(gps.lat_deg * 1000000.0);
    const int32_t gps_lon_micro = (int32_t)llround(gps.lon_deg * 1000000.0);
    h = fnv1a_u32(h, (uint32_t)gps_lat_micro);
    h = fnv1a_u32(h, (uint32_t)gps_lon_micro);
  }

  const int contact_total = the_mesh.getNumContacts();
  h = fnv1a_u32(h, (uint32_t)contact_total);
  for (int i = 0; i < contact_total; ++i) {
    ContactInfo c{};
    if (!the_mesh.getContactByIdx((uint32_t)i, c)) continue;
    for (size_t j = 0; j < sizeof(c.name); ++j) {
      h = fnv1a_u32(h, static_cast<uint8_t>(c.name[j]));
    }
    h = fnv1a_u32(h, (uint32_t)c.gps_lat);
    h = fnv1a_u32(h, (uint32_t)c.gps_lon);
  }
  return h;
}

MapPlotUi buildMapPlotUi(const GpsStatus& gps) {
  MapPlotUi out{};
#if defined(HELTEC_MAP_FIXED_TEST) && HELTEC_MAP_FIXED_TEST
  if (heltec::meshcore::ui::map::mapFixedTestEnabled()) {
    RawPoint raw[3];
    int raw_count = 0;
    int self_idx = 0;

    RawPoint& me = raw[raw_count++];
    me.lat_deg = HELTEC_MAP_FIXED_GPS_LAT;
    me.lon_deg = HELTEC_MAP_FIXED_GPS_LON;
    strncpy(me.label, "Me", sizeof(me.label));
    me.label[sizeof(me.label) - 1] = '\0';
    me.is_self = true;
    me.contact_index = -1;

    RawPoint& a = raw[raw_count++];
    a.lat_deg = HELTEC_MAP_FIXED_A_LAT;
    a.lon_deg = HELTEC_MAP_FIXED_A_LON;
    strncpy(a.label, "A", sizeof(a.label));
    a.label[sizeof(a.label) - 1] = '\0';
    a.is_self = false;
    a.contact_index = -1;

    RawPoint& b = raw[raw_count++];
    b.lat_deg = HELTEC_MAP_FIXED_B_LAT;
    b.lon_deg = HELTEC_MAP_FIXED_B_LON;
    strncpy(b.label, "B", sizeof(b.label));
    b.label[sizeof(b.label) - 1] = '\0';
    b.is_self = false;
    b.contact_index = -1;

    out.contact_gps_count = 2;
    out.drawable = true;
    compute_map_center(raw, raw_count, self_idx, out.center_lat, out.center_lon);

    uint32_t width_m = 50;
    uint32_t height_m = 50;
    for (int i = 0; i < raw_count && out.marker_count < MapPlotUi::kMaxMarkers; ++i) {
      MapPlotMarker& m = out.markers[out.marker_count++];
      strncpy(m.label, raw[i].label, sizeof(m.label));
      m.label[sizeof(m.label) - 1] = '\0';
      m.lat_deg = raw[i].lat_deg;
      m.lon_deg = raw[i].lon_deg;
      m.is_self = raw[i].is_self;
      m.contact_index = raw[i].contact_index;
      geo::offset_from_center(out.center_lat, out.center_lon, raw[i].lat_deg, raw[i].lon_deg, m.east_m,
                              m.north_m);
      width_m = (uint32_t)fmax((double)width_m, fabs((double)m.east_m) * 2.0);
      height_m = (uint32_t)fmax((double)height_m, fabs((double)m.north_m) * 2.0);
    }
    out.span_w_m = (uint32_t)((double)width_m * 1.1);
    out.span_h_m = (uint32_t)((double)height_m * 1.1);
    MAP_UI_LOG("fixed test G(%.6f,%.6f) A(%.6f,%.6f) B(%.6f,%.6f)", HELTEC_MAP_FIXED_GPS_LAT,
               HELTEC_MAP_FIXED_GPS_LON, HELTEC_MAP_FIXED_A_LAT, HELTEC_MAP_FIXED_A_LON,
               HELTEC_MAP_FIXED_B_LAT, HELTEC_MAP_FIXED_B_LON);
    return out;
  }
#endif
  RawPoint raw[MapPlotUi::kMaxMarkers];
  int raw_count = 0;
  int self_idx = -1;

  const bool gps_usable = usable_gps(gps);
  if (gps_usable && raw_count < MapPlotUi::kMaxMarkers) {
    RawPoint& p = raw[raw_count++];
    p.lat_deg = gps.lat_deg;
    p.lon_deg = gps.lon_deg;
    strncpy(p.label, "Me", sizeof(p.label));
    p.label[sizeof(p.label) - 1] = '\0';
    p.is_self = true;
    p.contact_index = -1;
    self_idx = raw_count - 1;
  }

  const int contact_total = the_mesh.getNumContacts();
  int contact_gps_total = 0;
  for (int i = 0; i < contact_total; ++i) {
    ContactInfo c{};
    if (!the_mesh.getContactByIdx((uint32_t)i, c)) continue;
    if (!geo::has_valid_gps_micro(c.gps_lat, c.gps_lon)) continue;

    const double lat_deg = geo::microdeg_to_deg(c.gps_lat);
    const double lon_deg = geo::microdeg_to_deg(c.gps_lon);
    if (!valid_geo_deg(lat_deg, lon_deg)) continue;
    ++contact_gps_total;
    if (raw_count >= MapPlotUi::kMaxMarkers) continue;

    RawPoint& p = raw[raw_count++];
    p.lat_deg = lat_deg;
    p.lon_deg = lon_deg;
    strncpy(p.label, c.name[0] ? c.name : "?", sizeof(p.label));
    p.label[sizeof(p.label) - 1] = '\0';
    p.is_self = false;
    p.contact_index = i;
  }

  out.contact_gps_count = contact_gps_total;
  out.drawable = true;

  if (raw_count > 0) {
    compute_map_center(raw, raw_count, self_idx, out.center_lat, out.center_lon);
  } else if (gps_usable) {
    out.center_lat = gps.lat_deg;
    out.center_lon = gps.lon_deg;
  }

  uint32_t width_m = 50;
  uint32_t height_m = 50;
  for (int i = 0; i < raw_count && out.marker_count < MapPlotUi::kMaxMarkers; ++i) {
    MapPlotMarker& m = out.markers[out.marker_count++];
    strncpy(m.label, raw[i].label, sizeof(m.label));
    m.label[sizeof(m.label) - 1] = '\0';
    m.lat_deg = raw[i].lat_deg;
    m.lon_deg = raw[i].lon_deg;
    m.is_self = raw[i].is_self;
    m.contact_index = raw[i].contact_index;
    if (raw_count > 1) {
      geo::offset_from_center(out.center_lat, out.center_lon, raw[i].lat_deg, raw[i].lon_deg, m.east_m,
                              m.north_m);
      width_m = (uint32_t)fmax((double)width_m, fabs((double)m.east_m) * 2.0);
      height_m = (uint32_t)fmax((double)height_m, fabs((double)m.north_m) * 2.0);
    }
  }
  out.span_w_m = (uint32_t)((double)width_m * 1.1);
  out.span_h_m = (uint32_t)((double)height_m * 1.1);

  return out;
}

}  // namespace

const MapPlotUi& MeshAppUi::mapPlotUi() const {
  const IBizFacade::GpsStatus gps = gpsStatus();
  const uint32_t fp = mapPlotInputFingerprint(gps);
  if (_map_plot_cache_valid && fp == _map_plot_fp) {
    return _map_plot_cache;
  }
  const uint32_t t0 = mapUiPerfNowUs();
  _map_plot_cache = buildMapPlotUi(gps);
  mapUiLogPerfDetail("buildMapPlotUi", t0, "markers=%d contacts_gps=%d",
                     _map_plot_cache.marker_count, _map_plot_cache.contact_gps_count);
  _map_plot_fp = fp;
  _map_plot_cache_valid = true;
  return _map_plot_cache;
}

}  // namespace heltec::meshcore::biz

#endif  // ENV_INCLUDE_MAP
