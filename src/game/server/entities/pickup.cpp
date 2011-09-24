/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include "pickup.h"

CPickup::CPickup(CGameWorld *pGameWorld, int Type)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_PICKUP)
{
	m_Type = Type;
	m_ProximityRadius = PickupPhysSize;

	Reset();

	GameWorld()->InsertEntity(this);
}

void CPickup::Reset()
{
	if (g_pData->m_aPickups[m_Type].m_Spawndelay > 0)
		m_SpawnTick = Server()->Tick() + Server()->TickSpeed() * g_pData->m_aPickups[m_Type].m_Spawndelay;
	else
		m_SpawnTick = -1;
}

void CPickup::Tick()
{
	// wait for respawn
	if(m_SpawnTick > 0)
	{
		if(Server()->Tick() > m_SpawnTick)
		{
			// respawn
			m_SpawnTick = -1;

			if(m_Type == POWERUP_WEAPON_SHOTGUN || m_Type == POWERUP_WEAPON_RIFLE || m_Type == POWERUP_WEAPON_GRENADE)
				GameServer()->CreateSound(m_Pos, GameServer()->m_Sound_WeaponSpawn.Get());
		}
		else
			return;
	}
	// Check if a player intersected us
	CCharacter *pChr = GameServer()->m_World.ClosestCharacter(m_Pos, 20.0f, 0);
	if(pChr && pChr->IsAlive())
	{
		// player picked us up, is someone was hooking us, let them go
		int RespawnTime = -1;
		switch (m_Type)
		{
			case POWERUP_HEALTH:
				if(pChr->IncreaseHealth(1))
				{
					GameServer()->CreateSound(m_Pos, GameServer()->m_Sound_PickupHealth.Get());
					RespawnTime = g_pData->m_aPickups[m_Type].m_Respawntime;
				}
				break;

			case POWERUP_ARMOR:
				if(pChr->IncreaseArmor(1))
				{
					GameServer()->CreateSound(m_Pos, GameServer()->m_Sound_PickupArmor.Get());
					RespawnTime = g_pData->m_aPickups[m_Type].m_Respawntime;
				}
				break;

			case POWERUP_WEAPON_RIFLE:
				if(pChr->GiveWeapon(WEAPON_RIFLE, 10))
				{
					RespawnTime = g_pData->m_aPickups[POWERUP_WEAPON_RIFLE].m_Respawntime;
					GameServer()->CreateSound(m_Pos, GameServer()->m_Sound_PickupShotgun.Get());
					GameServer()->SendWeaponPickup(pChr->GetPlayer()->GetCID(), WEAPON_RIFLE);
				}
				break;
			case POWERUP_WEAPON_GRENADE:
				if(pChr->GiveWeapon(WEAPON_GRENADE, 10))
				{
					RespawnTime = g_pData->m_aPickups[POWERUP_WEAPON_GRENADE].m_Respawntime;
					GameServer()->CreateSound(m_Pos, GameServer()->m_Sound_PickupGrenade.Get());
					GameServer()->SendWeaponPickup(pChr->GetPlayer()->GetCID(), WEAPON_GRENADE);
				}
				break;
			case POWERUP_WEAPON_SHOTGUN:
				if(pChr->GiveWeapon(WEAPON_SHOTGUN, 10))
				{
					RespawnTime = g_pData->m_aPickups[POWERUP_WEAPON_SHOTGUN].m_Respawntime;
					GameServer()->CreateSound(m_Pos, GameServer()->m_Sound_PickupShotgun.Get());
					GameServer()->SendWeaponPickup(pChr->GetPlayer()->GetCID(), WEAPON_SHOTGUN);
				}
				break;
			case POWERUP_NINJA:
				{
					// activate ninja on target player
					pChr->GiveNinja();
					RespawnTime = g_pData->m_aPickups[m_Type].m_Respawntime;

					// loop through all players, setting their emotes
					CCharacter *pC = static_cast<CCharacter *>(GameServer()->m_World.FindFirst(CGameWorld::ENTTYPE_CHARACTER));
					for(; pC; pC = (CCharacter *)pC->TypeNext())
					{
						if (pC != pChr)
							pC->SetEmote(EMOTE_SURPRISE, Server()->Tick() + Server()->TickSpeed());
					}

					pChr->SetEmote(EMOTE_ANGRY, Server()->Tick() + 1200 * Server()->TickSpeed() / 1000);
					break;
				}

			default:
				break;
		};

		if(RespawnTime >= 0)
		{
			char aBuf[256];
			str_format(aBuf, sizeof(aBuf), "pickup player='%d:%s' item=%d",
				pChr->GetPlayer()->GetCID(), Server()->ClientName(pChr->GetPlayer()->GetCID()), m_Type);
			GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);
			m_SpawnTick = Server()->Tick() + Server()->TickSpeed() * RespawnTime;
		}
	}
}

void CPickup::Snap(int SnappingClient)
{
	if(m_SpawnTick != -1 || NetworkClipped(SnappingClient))
		return;

	CNetObj_Pickup *pP = static_cast<CNetObj_Pickup *>(Server()->SnapNewItem(NETOBJTYPE_PICKUP, m_ID, sizeof(CNetObj_Pickup)));
	if(!pP)
		return;

	pP->m_X = (int)m_Pos.x;
	pP->m_Y = (int)m_Pos.y;
	pP->m_Type = m_Type;
}
