/*
 * Copyright (C) 2008-2019 TrinityCore <https://www.trinitycore.org/>
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

#ifndef SystemPackets_h__
#define SystemPackets_h__

#include "Packet.h"
#include "ObjectGuid.h"
#include "Optional.h"

namespace WorldPackets
{
    namespace Quest
    {
        struct QuestObjectiveCollect
        {
            QuestObjectiveCollect(int32 objectID = 0, int32 amount = 0, uint32 displayId = 0) : ObjectID(objectID), Amount(amount), DisplayID(displayId) { }
            uint32 ObjectID;
            uint32 Amount;
            uint32 DisplayID;
        };

        struct QuestCurrency
        {
            QuestCurrency(int32 currencyID = 0, int32 amount = 0) : CurrencyID(currencyID), Amount(amount) { }
            uint32 CurrencyID;
            uint32 Amount;
        };

        class QuestGiverRequestItems final : public ServerPacket
        {
        public:
            QuestGiverRequestItems() : ServerPacket(SMSG_QUEST_GIVER_REQUEST_ITEMS, 300) { }

            WorldPacket const* Write() override;

            ObjectGuid QuestGiverGUID;
            uint32 QuestID = 0;
            uint32 CompEmoteDelay = 0;
            uint32 CompEmoteType = 0;
            bool AutoLaunched = false;
            uint32 SuggestPartyMembers = 0;
            uint32 MoneyToGet = 0;
            std::vector<QuestObjectiveCollect> Collect;
            std::vector<QuestCurrency> Currency;
            uint32 StatusFlags[5] = { };
            uint32 QuestFlags = 0;
            std::string QuestTitle;
            std::string CompletionText;
        };
    }
}

#endif // SystemPackets_h__