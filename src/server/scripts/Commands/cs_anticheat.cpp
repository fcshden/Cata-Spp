/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "AccountMgr.h"
#include "Language.h"
#include "ScriptMgr.h"
#include "ObjectMgr.h"
#include "ObjectAccessor.h"
#include "Chat.h"
#include "AnticheatMgr.h"
#include "Player.h"
#include "World.h"
#include "WorldSession.h"
#include "SpellAuras.h"

using namespace Trinity::ChatCommands;

enum Spells
{
    SHACKLES = 38505,
    LFG_SPELL_DUNGEON_DESERTER = 71041,
    BG_SPELL_DESERTER = 26013,
    SILENCED = 23207
};

class anticheat_commandscript : public CommandScript
{
public:
    anticheat_commandscript() : CommandScript("anticheat_commandscript") { }

    std::vector<ChatCommand> GetCommands() const override
    {
        static std::vector<ChatCommand> anticheatCommandTable =
        {
            { "global",      rbac::RBAC_PERM_COMMAND_GM_CHAT,                 true,  HandleAntiCheatGlobalCommand,             "" },
            { "player",      rbac::RBAC_PERM_COMMAND_GM_CHAT,                 true,  HandleAntiCheatPlayerCommand,             "" },
            { "delete",      rbac::RBAC_PERM_COMMAND_GM_CHAT,                 true,  HandleAntiCheatDeleteCommand,             "" },
            { "purge",       rbac::RBAC_PERM_COMMAND_GM_CHAT,                 true,  HandleAntiCheatPurgeCommand,              "" },
            { "handle",      rbac::RBAC_PERM_COMMAND_GM_CHAT,                 true,  HandleAntiCheatHandleCommand,             "" },
            { "jail",        rbac::RBAC_PERM_COMMAND_GM_CHAT,                 true,  HandleAnticheatJailCommand,               "" },
            { "parole",      rbac::RBAC_PERM_COMMAND_GM_CHAT,                 true,  HandleAnticheatParoleCommand,             "" }
        };

        static std::vector<ChatCommand> commandTable =
        {
            { "anticheat",   rbac::RBAC_PERM_COMMAND_GM_CHAT,                 true,  nullptr,                                  "", anticheatCommandTable },
        };

        return commandTable;
    }

    static bool HandleAnticheatJailCommand(ChatHandler* handler, char const* args)
    {
        if (!sWorld->getBoolConfig(CONFIG_ANTICHEAT_ENABLE))
            return false;

        if (!*args)
            return false;

        Player* target;
        if (!handler->extractPlayerTarget((char*)args, &target))
            return false;

        // teleport both to jail.
        if (!handler->GetSession()->GetSecurity() == SEC_CONSOLE)
        {
            handler->GetSession()->GetPlayer()->TeleportTo(1, 16226.5f, 16403.6f, -64.5f, 3.2f);
        }

        WorldLocation jailLoc = WorldLocation(1, 16226.5f, 16403.6f, -64.5f, 3.2f);// GM Jail Location
        target->TeleportTo(jailLoc);
        target->SetHomebind(jailLoc, 876);// GM Jail Homebind location
        target->CastSpell(target, SHACKLES);// shackle him in place to ensure no exploit happens for jail break attempt
        if (Aura* dungdesert = target->AddAura(LFG_SPELL_DUNGEON_DESERTER, target))// LFG_SPELL_DUNGEON_DESERTER
        {
            dungdesert->SetDuration(-1);
        }
        if (Aura* bgdesert = target->AddAura(BG_SPELL_DESERTER, target))// BG_SPELL_DESERTER
        {
            bgdesert->SetDuration(-1);
        }
        if (Aura* silent = target->AddAura(SILENCED, target))// SILENCED
        {
            silent->SetDuration(-1);
        }

        return true;
    }

    static bool HandleAnticheatParoleCommand(ChatHandler* handler, char const* args)
    {
        if (!sWorld->getBoolConfig(CONFIG_ANTICHEAT_ENABLE))
            return false;

        if (!*args)
            return false;

        Player* target;
        if (!handler->extractPlayerTarget((char*)args, &target))
            return false;

        WorldLocation Aloc = WorldLocation(0, -8833.37f, 628.62f, 94.00f, 1.06f);// Stormwind
        WorldLocation Hloc = WorldLocation(1, 1569.59f, -4397.63f, 16.06f, 0.54f);// Orgrimmar

        if (target->GetTeamId() == TEAM_ALLIANCE)
        {
            target->TeleportTo(0, -8833.37f, 628.62f, 94.00f, 1.06f);//Stormwind
            target->SetHomebind(Aloc, 1519);// Stormwind Homebind location
        }
        else
        {
            target->TeleportTo(1, 1569.59f, -4397.63f, 7.7f, 0.54f);//Orgrimmar
            target->SetHomebind(Hloc, 1653);// Orgrimmar Homebind location
        }
        target->RemoveAura(SHACKLES);// remove shackles
        target->RemoveAura(LFG_SPELL_DUNGEON_DESERTER);// LFG_SPELL_DUNGEON_DESERTER
        target->RemoveAura(BG_SPELL_DESERTER);// BG_SPELL_DESERTER
        target->RemoveAura(SILENCED);// SILENCED
        sAnticheatMgr->AnticheatDeleteCommand(target->GetGUID());// deletes auto reports on player
        return true;
    }

    static bool HandleAntiCheatDeleteCommand(ChatHandler* handler, char const* args)
    {
        if (!sWorld->getBoolConfig(CONFIG_ANTICHEAT_ENABLE))
            return false;

        if (!*args)
            return false;

        Player* target;
        if (!handler->extractPlayerTarget((char*)args, &target))
            return false;

        sAnticheatMgr->AnticheatDeleteCommand(target->GetGUID());
        handler->PSendSysMessage("Anticheat players_reports_status deleted for player %s", target->GetName().c_str());
        return true;
    }
    static bool HandleAntiCheatPurgeCommand(ChatHandler* handler)
    {
        // For the sins I am about to commit, may CTHULHU forgive me
        // this will purge the daily_player_reports which is the cumlative statistics of auto reports
        sAnticheatMgr->AnticheatPurgeCommand(handler);
        handler->PSendSysMessage("The Anticheat daily_player_reports has been purged.");
        return true;
    }

    static bool HandleAntiCheatPlayerCommand(ChatHandler* handler, char const* args)
    {
        if (!sWorld->getBoolConfig(CONFIG_ANTICHEAT_ENABLE))
            return false;

        if (!*args)
            return false;

        Player* target;
        if (!handler->extractPlayerTarget((char*)args, &target))
            return false;

        uint32 guid = target->GetGUID().GetCounter();

        float average = sAnticheatMgr->GetAverage(guid);
        uint32 total_reports = sAnticheatMgr->GetTotalReports(guid);
        uint32 speed_reports = sAnticheatMgr->GetTypeReports(guid,0);
        uint32 fly_reports = sAnticheatMgr->GetTypeReports(guid,1);
        uint32 jump_reports = sAnticheatMgr->GetTypeReports(guid,3);
        uint32 waterwalk_reports = sAnticheatMgr->GetTypeReports(guid,2);
        uint32 teleportplane_reports = sAnticheatMgr->GetTypeReports(guid,4);
        uint32 climb_reports = sAnticheatMgr->GetTypeReports(guid,5);
        uint32 teleport_reports = sAnticheatMgr->GetTypeReports(guid, 6);
        uint32 ignorecontrol_reports = sAnticheatMgr->GetTypeReports(guid, 7);
        uint32 zaxis_reports = sAnticheatMgr->GetTypeReports(guid, 8);
        uint32 antiswim_reports = sAnticheatMgr->GetTypeReports(guid, 9);
        uint32 gravity_reports = sAnticheatMgr->GetTypeReports(guid, 10);
        uint32 antiknockback_reports = sAnticheatMgr->GetTypeReports(guid, 11);

        uint32 latency = 0;
        latency = target->GetSession()->GetLatency();

        handler->PSendSysMessage("Information about player %s || Latency %u ms", target->GetName().c_str(), latency);
        handler->PSendSysMessage("Average: %f || Total Reports: %u ", average, total_reports);
        handler->PSendSysMessage("Speed Reports: %u || Fly Reports: %u || Jump Reports: %u ", speed_reports, fly_reports, jump_reports);
        handler->PSendSysMessage("Walk On Water Reports: %u  || Teleport To Plane Reports: %u", waterwalk_reports, teleportplane_reports);
        handler->PSendSysMessage("Teleport Reports: %u || Climb Reports: %u", teleport_reports, climb_reports);
        handler->PSendSysMessage("Ignore Control Reports: %u || Ignore Z-Axis Reports: %u", ignorecontrol_reports, zaxis_reports);
        handler->PSendSysMessage("Ignore Anti-Swim Reports: %u || Gravity Reports: %u", antiswim_reports, gravity_reports);
        handler->PSendSysMessage("Anti-Knock Back Reports: %u", antiknockback_reports);
        return true;
    }

    static bool HandleAntiCheatHandleCommand(ChatHandler* handler, bool enable)
    {
        if (enable)
        {
            sWorld->setBoolConfig(CONFIG_ANTICHEAT_ENABLE,true);
            handler->SendSysMessage("The Anticheat System is now: Enabled!");
        }
        else
        {
            sWorld->setBoolConfig(CONFIG_ANTICHEAT_ENABLE,false);
            handler->SendSysMessage("The Anticheat System is now: Disabled!");
        }

        return true;
    }

    static bool HandleAntiCheatGlobalCommand(ChatHandler* handler)
    {
        if (!sWorld->getBoolConfig(CONFIG_ANTICHEAT_ENABLE))
        {
            handler->PSendSysMessage("The Anticheat System is disabled.");
            return true;
        }

        sAnticheatMgr->AnticheatGlobalCommand(handler);

        return true;
    }
};

void AddSC_anticheat_commandscript()
{
    new anticheat_commandscript();
}
