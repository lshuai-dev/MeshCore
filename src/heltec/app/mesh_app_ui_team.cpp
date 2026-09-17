/**
 * @file    mesh_app_ui_team.cpp
 * @brief   Small UI facade for Team creation and pairing.
 * @version 1.0
 * @date    2026-09-04
 */
#include "HeltecMesh.h"
#include "mesh_app_ui.hpp"
#include "ui/core/ht_event.h"

#include <string.h>
#include <stdio.h>

namespace heltec::meshcore::biz
{

IBizFacade::TeamStatus MeshAppUi::teamStatus() const
{
    TeamStatus status{};
#if defined(ENV_INCLUDE_GPS) && ENV_INCLUDE_GPS
    status.is_leader = the_mesh.teamService().role() == heltec::meshcore::team::TeamRole::Leader;
    status.has_team = the_mesh.teamService().hasTeam();
    status.member_count = static_cast<int>(the_mesh.teamService().memberCount());
    status.member_revision = the_mesh.teamService().memberRevision();
    status.pair_state = static_cast<int>(the_mesh.teamJoinService().state());
    status.join_window_open = the_mesh.teamJoinService().joinWindowOpen();
    if (status.is_leader)
    {
        heltec::meshcore::team::PendingJoin pending{};
        if (the_mesh.teamJoinService().pendingJoin(pending))
        {
            status.pairing_code = pending.pairing_code;
            status.pending_session = pending.session_id;
        }
    }
    else
    {
        status.pairing_code = the_mesh.teamJoinService().pairingCode();
    }
#endif
    return status;
}

int MeshAppUi::teamOnlineCount() const
{
#if defined(ENV_INCLUDE_GPS) && ENV_INCLUDE_GPS
    int online = 0;
    const uint32_t now_ms = millis();
    const auto &team = the_mesh.teamService();
    for (size_t slot = 0; slot < HELTEC_TEAM_MAX_MEMBERS; ++slot)
    {
        const heltec::meshcore::team::TeamMember *const member = team.memberSlot(slot);
        if (member && now_ms - member->last_seen_ms <= HELTEC_TEAM_MEMBER_TIMEOUT_SEC * 1000UL)
        {
            ++online;
        }
    }
    return online;
#else
    return 0;
#endif
}

bool MeshAppUi::teamPendingDetails(char *name, size_t length, uint64_t &session,
                                   uint16_t &code) const
{
    session = 0;
    code = 0;
#if defined(ENV_INCLUDE_GPS) && ENV_INCLUDE_GPS
    if (the_mesh.teamService().role() != heltec::meshcore::team::TeamRole::Leader)
        return false;
    if (!name || length == 0)
        return false;
    heltec::meshcore::team::PendingJoin pending{};
    if (!the_mesh.teamJoinService().pendingJoin(pending))
        return false;
    snprintf(name, length, "%.*s", pending.name_len, pending.name);
    session = pending.session_id;
    code = pending.pairing_code;
    return true;
#else
    if (name && length > 0)
        name[0] = '\0';
    return false;
#endif
}

int MeshAppUi::teamOfferCount() const
{
#if defined(ENV_INCLUDE_GPS) && ENV_INCLUDE_GPS
    return static_cast<int>(the_mesh.teamJoinService().offerCount());
#else
    return 0;
#endif
}

bool MeshAppUi::teamOfferLabel(int index, char *label, size_t length) const
{
#if defined(ENV_INCLUDE_GPS) && ENV_INCLUDE_GPS
    if (the_mesh.teamService().role() != heltec::meshcore::team::TeamRole::Member)
        return false;
    if (!label || length == 0 || index < 0)
        return false;
    heltec::meshcore::team::TeamOffer offer{};
    if (!the_mesh.teamJoinService().offerAt(static_cast<size_t>(index), offer))
        return false;
    snprintf(label, length, "%.*s", offer.name_len, offer.name);
    return true;
#else
    (void)index;
    if (label && length > 0)
        label[0] = '\0';
    return false;
#endif
}

bool MeshAppUi::teamSelectOffer(int index)
{
#if defined(ENV_INCLUDE_GPS) && ENV_INCLUDE_GPS
    if (the_mesh.teamService().role() != heltec::meshcore::team::TeamRole::Member)
        return false;
    return index >= 0 && the_mesh.teamJoinService().selectOffer(static_cast<size_t>(index));
#else
    (void)index;
    return false;
#endif
}

bool MeshAppUi::teamStartSearch()
{
#if defined(ENV_INCLUDE_GPS) && ENV_INCLUDE_GPS
    if (the_mesh.teamService().role() != heltec::meshcore::team::TeamRole::Member)
        return false;
    return the_mesh.teamJoinService().discoverTeams();
#else
    return false;
#endif
}

bool MeshAppUi::teamSetRole(bool leader)
{
#if defined(ENV_INCLUDE_GPS) && ENV_INCLUDE_GPS
    teamCancel();
    (void)teamLeave();
    const auto role = leader ? heltec::meshcore::team::TeamRole::Leader
                             : heltec::meshcore::team::TeamRole::Member;
    const bool changed = the_mesh.teamService().setRole(role);
    notifyAppState(heltec::meshcore::ui::AppStateEventType::TeamChanged);
    notifyAppState(heltec::meshcore::ui::AppStateEventType::FindFriendChanged);
    return changed;
#else
    (void)leader;
    return false;
#endif
}

bool MeshAppUi::teamSetActive(bool active)
{
#if defined(ENV_INCLUDE_GPS) && ENV_INCLUDE_GPS
    if (the_mesh.teamService().role() != heltec::meshcore::team::TeamRole::Leader)
        return false;
    if (!active)
        return teamLeave();
    auto &team = the_mesh.teamService();
    return team.hasTeam() ? team.setEnabled(true) : team.createTeam(the_mesh.getNodeName());
#else
    (void)active;
    return false;
#endif
}

bool MeshAppUi::teamSetJoining(bool open)
{
#if defined(ENV_INCLUDE_GPS) && ENV_INCLUDE_GPS
    if (the_mesh.teamService().role() != heltec::meshcore::team::TeamRole::Leader)
        return false;
    if (!the_mesh.teamService().enabled())
        return false;
    auto &join = the_mesh.teamJoinService();
    if (open)
        return join.joinWindowOpen() || join.openJoinWindow();
    join.closeJoinWindow();
    return true;
#else
    (void)open;
    return false;
#endif
}

bool MeshAppUi::teamMessageTarget(int member_index, ui::UiSendMessageTarget &target) const
{
    target = ui::UiSendMessageTarget{};
#if defined(ENV_INCLUDE_GPS) && ENV_INCLUDE_GPS
    const auto &team = the_mesh.teamService();
    if (!team.enabled())
        return false;
    if (member_index == -1)
    {
        target.kind = ui::UiMessageTargetKind::Channel;
        target.channel_idx = HELTEC_TEAM_CHANNEL_INDEX;
        snprintf(target.label, sizeof(target.label), "%s", team.credentials().name);
        return true;
    }
    heltec::meshcore::team::TeamMember member{};
    if (member_index < 0 || !team.memberAt(static_cast<size_t>(member_index), member))
        return false;
    target.kind = ui::UiMessageTargetKind::Direct;
    memcpy(target.pub_key_prefix, member.public_key, sizeof(target.pub_key_prefix));
    snprintf(target.label, sizeof(target.label), "%.*s", member.name_len, member.name);
    return true;
#else
    (void)member_index;
    return false;
#endif
}

bool MeshAppUi::mapMessageTarget(const uint8_t *prefix, size_t length,
                                  ui::UiSendMessageTarget &target) const
{
    target = ui::UiSendMessageTarget{};
    if (!prefix || length != sizeof(target.pub_key_prefix))
        return false;
#if defined(ENV_INCLUDE_GPS) && ENV_INCLUDE_GPS
    const auto &team = the_mesh.teamService();
    for (size_t slot = 0; slot < HELTEC_TEAM_MAX_MEMBERS; ++slot)
    {
        const heltec::meshcore::team::TeamMember *const member = team.memberSlot(slot);
        if (!member || memcmp(member->public_key, prefix, length) != 0)
            continue;
        target.kind = ui::UiMessageTargetKind::Direct;
        memcpy(target.pub_key_prefix, member->public_key, sizeof(target.pub_key_prefix));
        snprintf(target.label, sizeof(target.label), "%.*s", member->name_len, member->name);
        return true;
    }
#endif
    // Resolve identity rather than a mutable visible-list ordinal.
    const ContactInfo *peer = the_mesh.lookupMessagePeer(prefix, length);
    if (!peer)
        return false;
    memcpy(target.pub_key_prefix, peer->id.pub_key, sizeof(target.pub_key_prefix));
    snprintf(target.label, sizeof(target.label), "%.*s",
             static_cast<int>(sizeof(peer->name)), peer->name);
    return true;
}

bool MeshAppUi::teamDecidePending(uint64_t session, bool approve)
{
#if defined(ENV_INCLUDE_GPS) && ENV_INCLUDE_GPS
    if (the_mesh.teamService().role() != heltec::meshcore::team::TeamRole::Leader)
        return false;
    return approve ? the_mesh.teamJoinService().approvePending(session) :
                     the_mesh.teamJoinService().rejectPending(session);
#else
    (void)session;
    (void)approve;
    return false;
#endif
}

bool MeshAppUi::teamLeave()
{
#if defined(ENV_INCLUDE_GPS) && ENV_INCLUDE_GPS
    the_mesh.teamJoinService().closeJoinWindow();
    return the_mesh.teamService().leaveTeam();
#else
    return false;
#endif
}

void MeshAppUi::teamCancel()
{
#if defined(ENV_INCLUDE_GPS) && ENV_INCLUDE_GPS
    the_mesh.teamJoinService().cancel();
#endif
}

} // namespace heltec::meshcore::biz
