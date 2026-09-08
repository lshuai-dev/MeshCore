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

bool MeshAppUi::teamIsLeader() const
{
#if HELTEC_TEAM_FEATURE
    return HELTEC_TEAM_ROLE == 1;
#else
    return false;
#endif
}

bool MeshAppUi::teamHasTeam() const
{
#if HELTEC_TEAM_FEATURE
    return the_mesh.teamService().hasTeam();
#else
    return false;
#endif
}

int MeshAppUi::teamMemberCount() const
{
#if HELTEC_TEAM_FEATURE
    return static_cast<int>(the_mesh.teamService().memberCount());
#else
    return 0;
#endif
}

int MeshAppUi::teamOnlineCount() const
{
#if HELTEC_TEAM_FEATURE
    int online = 0;
    const size_t count = the_mesh.teamService().memberCount();
    for (size_t index = 0; index < count; ++index)
    {
        heltec::meshcore::team::TeamMember member{};
        if (the_mesh.teamService().memberAt(index, member) &&
            millis() - member.last_seen_ms <= 20UL * 60UL * 1000UL)
        {
            ++online;
        }
    }
    return online;
#else
    return 0;
#endif
}

int MeshAppUi::teamPairState() const
{
#if HELTEC_TEAM_FEATURE
    return static_cast<int>(the_mesh.teamJoinService().state());
#else
    return 0;
#endif
}

uint16_t MeshAppUi::teamPairingCode() const
{
#if HELTEC_TEAM_FEATURE && HELTEC_TEAM_ROLE == 1
    heltec::meshcore::team::PendingJoin pending{};
    return the_mesh.teamJoinService().pendingJoin(pending) ? pending.pairing_code : 0;
#elif HELTEC_TEAM_FEATURE
    return the_mesh.teamJoinService().pairingCode();
#else
    return 0;
#endif
}

bool MeshAppUi::teamPendingDetails(char *name, size_t length, uint64_t &session,
                                   uint16_t &code) const
{
    session = 0;
    code = 0;
#if HELTEC_TEAM_FEATURE && HELTEC_TEAM_ROLE == 1
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
#if HELTEC_TEAM_FEATURE
    return static_cast<int>(the_mesh.teamJoinService().offerCount());
#else
    return 0;
#endif
}

bool MeshAppUi::teamOfferLabel(int index, char *label, size_t length) const
{
#if HELTEC_TEAM_FEATURE && HELTEC_TEAM_ROLE == 0
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
#if HELTEC_TEAM_FEATURE && HELTEC_TEAM_ROLE == 0
    return index >= 0 && the_mesh.teamJoinService().selectOffer(static_cast<size_t>(index));
#else
    (void)index;
    return false;
#endif
}

bool MeshAppUi::teamPrimaryAction()
{
#if !HELTEC_TEAM_FEATURE
    return false;
#elif HELTEC_TEAM_ROLE == 1
    auto &team = the_mesh.teamService();
    auto &join = the_mesh.teamJoinService();
    if (!team.hasTeam())
    {
        char name[heltec::meshcore::team::kMaxNameBytes + 1] = {};
        snprintf(name, sizeof(name), "%s", nodeName());
        if (!team.createTeam(name))
            return false;
    }
    return join.joinWindowOpen() || join.openJoinWindow();
#else
    auto &join = the_mesh.teamJoinService();
    if (join.offerCount() > 0)
        return join.selectOffer(0);
    return join.discoverTeams();
#endif
}

bool MeshAppUi::teamSetActive(bool active)
{
#if HELTEC_TEAM_FEATURE && HELTEC_TEAM_ROLE == 1
    if (!active)
        return teamLeave();
    auto &team = the_mesh.teamService();
    return team.hasTeam() ? team.setEnabled(true) : team.createTeam(nodeName());
#else
    (void)active;
    return false;
#endif
}

bool MeshAppUi::teamSetJoining(bool open)
{
#if HELTEC_TEAM_FEATURE && HELTEC_TEAM_ROLE == 1
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
#if HELTEC_TEAM_FEATURE
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
    // Resolve identity rather than a mutable visible-list ordinal.
    const ContactInfo *peer = the_mesh.lookupMessagePeer(prefix, length);
    if (!peer)
        return false;
    memcpy(target.pub_key_prefix, peer->id.pub_key, sizeof(target.pub_key_prefix));
    snprintf(target.label, sizeof(target.label), "%.*s",
             static_cast<int>(sizeof(peer->name)), peer->name);
    return true;
}

uint64_t MeshAppUi::teamPendingSession() const
{
#if HELTEC_TEAM_FEATURE && HELTEC_TEAM_ROLE == 1
    heltec::meshcore::team::PendingJoin pending{};
    return the_mesh.teamJoinService().pendingJoin(pending) ? pending.session_id : 0;
#else
    return 0;
#endif
}

bool MeshAppUi::teamJoinWindowOpen() const
{
#if HELTEC_TEAM_FEATURE
    return the_mesh.teamJoinService().joinWindowOpen();
#else
    return false;
#endif
}

bool MeshAppUi::teamDecidePending(uint64_t session, bool approve)
{
#if HELTEC_TEAM_FEATURE && HELTEC_TEAM_ROLE == 1
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
#if HELTEC_TEAM_FEATURE
    the_mesh.teamJoinService().cancel();
    return the_mesh.teamService().leaveTeam();
#else
    return false;
#endif
}

void MeshAppUi::teamCancel()
{
#if HELTEC_TEAM_FEATURE
    the_mesh.teamJoinService().cancel();
#endif
}

} // namespace heltec::meshcore::biz
