#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedEscortAI.h"
#include "SpellScript.h"
#include "Vehicle.h"
#include "zone_wandering_isle.h"

class npc_jaomin_ro : public CreatureScript
{
public:
    npc_jaomin_ro() : CreatureScript("npc_jaomin_ro") { }
    
    struct npc_jaomin_roAI : public ScriptedAI
    {
        npc_jaomin_roAI(Creature * creature) : ScriptedAI(creature)
        {
            me->SetReactState(REACT_DEFENSIVE);
            me->SetDisplayId(39755);
            me->setFaction(14);
        }

        uint32 jaomin_jump_timer;
        uint32 hit_circle_timer;
        uint32 falcon_timer;
        uint32 reset_timer;
		uint32 check_area_timer;

        bool is_in_falcon;

        void Reset()
        {
            is_in_falcon = false;

            me->SetReactState(REACT_DEFENSIVE);
            me->SetDisplayId(39755);
            me->setFaction(2357);
            me->CombatStop(true);
            me->GetMotionMaster()->MovePoint(1, 1380.35f, 3170.68f, 136.93f);
        }

        void EnterCombat(Unit * /*target*/)
        {
            Talk(SAY_AGGRO);

			jaomin_jump_timer = 1000;
			hit_circle_timer = 2000;
			check_area_timer = 2500;
        }
        
        void DamageTaken(Unit * /*attacker*/, uint32 & damage)
        {
			if (damage >= me->GetHealth())
			{
				damage = me->GetHealth() - 1;
			}

            if (me->HealthBelowPctDamaged(30, damage) && !is_in_falcon)
            {
                is_in_falcon = true;

                me->SetDisplayId(39796);

				falcon_timer = 1000;
            }

            if (me->HealthBelowPctDamaged(5, damage))
            {
				Talk(SAY_VICTORY);

                me->SetDisplayId(39755);
				me->AttackStop();
				me->CombatStop();
				me->setFaction(35);
				me->HandleEmoteCommand(EMOTE_ONESHOT_SALUTE);

                std::list<Player*> player_list;
                
				GetPlayerListInGrid(player_list, me, 10.0f);

				for (auto player : player_list)
				{
					player->KilledMonsterCredit(me->GetEntry(), 0);
				}

				reset_timer = 5000;
            }
        }
        
        void UpdateAI(const uint32 diff)
		{
			if (!UpdateVictim())
			{
				events.Update(diff);
			}

			if (me->HasUnitState(UNIT_STATE_CASTING))
			{
				return;
			}

			if (jaomin_jump_timer <= diff)
			{
				if (is_in_falcon)
				{
					return;
				}

				DoCastVictim(108938, true);

				jaomin_jump_timer = 30000;
			}

			else
			{
				jaomin_jump_timer -= diff;
			}

			if (hit_circle_timer <= diff)
			{
				if (is_in_falcon)
				{
					return;
				}

				DoCastVictim(119301, true);

				hit_circle_timer = 3000;
			}

			else
			{
				hit_circle_timer -= diff;
			}

			if (falcon_timer <= diff)
			{
				DoCastVictim(108935, true);

				falcon_timer = 4000;
			}

			else
			{
				falcon_timer -= diff;
			}

			if (reset_timer <= diff)
			{
				Reset();
			}

			else
			{
				reset_timer -= diff;
			}

			if (check_area_timer <= diff)
			{
				if (me->GetAreaId() != 5843)
				{
					Reset();
				}

				check_area_timer = 2500;
			}

			else
			{
				check_area_timer -= diff;
			}
            
            DoMeleeAttackIfReady();
        }
    };

    CreatureAI * GetAI(Creature * creature) const
    {
        return new npc_jaomin_roAI(creature);
    }
};

class mob_master_shang_xi : public CreatureScript
{
    public:
        mob_master_shang_xi() : CreatureScript("mob_master_shang_xi") { }

        bool OnQuestAccept(Player* player, Creature* creature, Quest const* quest)
        {
            if (quest->GetQuestId() == 29408) // La lecon du parchemin brulant
            {
                creature->AddAura(114610, creature);
                creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP | UNIT_NPC_FLAG_QUESTGIVER);
                creature->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
            }

            return true;
        }

        CreatureAI* GetAI(Creature* creature) const
        {
            return new mob_master_shang_xi_AI(creature);
        }

        struct mob_master_shang_xi_AI : public ScriptedAI
        {
            mob_master_shang_xi_AI(Creature* creature) : ScriptedAI(creature)
            {
                checkPlayersTime = 2000;
            }

            uint32 checkPlayersTime;

            void SpellHit(Unit* caster, const SpellInfo* pSpell)
            {
                if (pSpell->Id == 114746) // Attraper la flamme
                {
                    if (caster->GetTypeId() == TYPEID_PLAYER)
                    {
                        if (caster->ToPlayer()->GetQuestStatus(29408) == QUEST_STATUS_INCOMPLETE)
                        {
                            me->CastSpell(caster, 114611, true);
                            me->RemoveAurasDueToSpell(114610);
                            me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP | UNIT_NPC_FLAG_QUESTGIVER);
                            me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
                        }
                    }
                }
            }

            void UpdateAI(const uint32 diff)
            {
                if (checkPlayersTime <= diff)
                {
                    std::list<Player*> playerList;
                    GetPlayerListInGrid(playerList, me, 5.0f);

                    bool playerWithQuestNear = false;

                    for (auto player: playerList)
                        if (player->GetQuestStatus(29408) == QUEST_STATUS_INCOMPLETE)
                            if (!player->HasItemCount(80212))// Flamme du maitre
                                playerWithQuestNear = true;

                    if (playerWithQuestNear && !me->HasAura(114610))
                    {
                        me->AddAura(114610, me);
                        me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP | UNIT_NPC_FLAG_QUESTGIVER);
                        me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
                    }
                    else if (!playerWithQuestNear && me->HasAura(114610))
                    {
                        me->RemoveAurasDueToSpell(114610);
                        me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP | UNIT_NPC_FLAG_QUESTGIVER);
                        me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
                    }

                    checkPlayersTime = 2000;
                }
                else
                    checkPlayersTime -= diff;
            }
        };
};

class go_wandering_weapon_rack : public GameObjectScript
{
public:
    go_wandering_weapon_rack() : GameObjectScript("go_wandering_weapon_rack") { }

    bool OnGossipHello(Player* player, GameObject* go)
    {
        if (player->GetQuestStatus(30027) == QUEST_STATUS_INCOMPLETE && !player->HasItemCount(73209))
        {
            player->AddItem(73209, 1);
        }
        else if (player->GetQuestStatus(30033) == QUEST_STATUS_INCOMPLETE && (!player->HasItemCount(76392) || !player->HasItemCount(76390)))
        {
            player->AddItem(76392, 1);
            player->AddItem(76390, 1);
        }
        else if (player->GetQuestStatus(30034) == QUEST_STATUS_INCOMPLETE && !player->HasItemCount(73211))
        {
            player->AddItem(73211, 1);
        }
        else if (player->GetQuestStatus(30035) == QUEST_STATUS_INCOMPLETE && (!player->HasItemCount(76393) || !player->HasItemCount(73207)))
        {
            player->AddItem(76393, 1);
            player->AddItem(73207, 1);
        }
        else if (player->GetQuestStatus(30036) == QUEST_STATUS_INCOMPLETE && (!player->HasItemCount(73212) || !player->HasItemCount(73208)))
        {
            player->AddItem(73212, 1);
            player->AddItem(73208, 1);
        }
        else if (player->GetQuestStatus(30037) == QUEST_STATUS_INCOMPLETE && (!player->HasItemCount(73213) || !player->HasItemCount(76391)))
        {
            player->AddItem(73213, 1);
            player->AddItem(76391, 1);
        }
        else if (player->GetQuestStatus(30038) == QUEST_STATUS_INCOMPLETE && !player->HasItemCount(73210))
        {
            player->AddItem(73210, 1);
        }

        return true;
    }
};

class mob_training_target : public CreatureScript
{
public:
    mob_training_target() : CreatureScript("mob_training_target") { }
    CreatureAI* GetAI(Creature* creature) const
    {
        return new mob_training_targetAI(creature);
    }
    
    struct mob_training_targetAI : public ScriptedAI
    {
    	mob_training_targetAI(Creature* creature) : ScriptedAI(creature) {}
    	
        void Reset()
        {
            me->SetReactState(REACT_PASSIVE);
        }

        void EnterCombat()
        {
            return;
        }
    };
};

class mob_tushui_trainee : public CreatureScript
{
    public:
        mob_tushui_trainee() : CreatureScript("mob_tushui_trainee") { }

        CreatureAI* GetAI(Creature* creature) const
        {
            return new mob_tushui_trainee_AI(creature);
        }

        struct mob_tushui_trainee_AI : public ScriptedAI
        {
            mob_tushui_trainee_AI(Creature* creature) : ScriptedAI(creature) {}

            void Reset()
            {
                me->SetReactState(REACT_DEFENSIVE);
                me->setFaction(2357);
            }

            void DamageTaken(Unit* attacker, uint32& damage)
            {
                if (me->HealthBelowPctDamaged(5, damage))
                {
                    if(attacker && attacker->GetTypeId() == TYPEID_PLAYER)
                        attacker->ToPlayer()->KilledMonsterCredit(54586, 0);
                    me->CombatStop();
                    me->SetFullHealth();
                    me->HandleEmoteCommand(EMOTE_ONESHOT_SALUTE);
                    me->setFaction(35);
                    me->DespawnOrUnsummon(3000);
                    damage = 0;
                }
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                DoMeleeAttackIfReady();
            }
        };
};

class mob_attacker_dimwind : public CreatureScript
{
public:
    mob_attacker_dimwind() : CreatureScript("mob_attacker_dimwind") { }
    CreatureAI* GetAI(Creature* creature) const
    {
        return new mob_attacker_dimwindAI(creature);
    }
    
    struct mob_attacker_dimwindAI : public ScriptedAI
    {
    	mob_attacker_dimwindAI(Creature* creature) : ScriptedAI(creature) {}
    	
        void DamageTaken(Unit* pDoneBy, uint32 &uiDamage)
        {
            if(me->GetHealthPct() < 90 && pDoneBy && pDoneBy->ToCreature() && pDoneBy->ToCreature()->GetEntry() == 54785)
                uiDamage = 0;
        }
    };
};

class mob_min_dimwind : public CreatureScript
{
public:
    mob_min_dimwind() : CreatureScript("mob_min_dimwind") { }
    
    CreatureAI* GetAI(Creature* creature) const
    {
        return new mob_min_dimwindAI(creature);
    }
    
    struct mob_min_dimwindAI : public ScriptedAI
    {
        EventMap events;
        uint64 guidMob[4];

        enum eEvents
        {
            EVENT_CHECK_MOBS    = 1,
            EVENT_RESET         = 2
        };
        
        mob_min_dimwindAI(Creature* creature) : ScriptedAI(creature)
        {
            for(int i = 0; i < 4; i++)
                guidMob[i] = 0;

            ResetMobs();
            me->HandleEmoteCommand(EMOTE_STATE_READY2H);
        }
        
        void DamageTaken(Unit* pDoneBy, uint32 &uiDamage)
        {
            if(me->GetHealthPct() < 25 && pDoneBy && pDoneBy->ToCreature() && pDoneBy->ToCreature()->GetEntry() == 54130)
                uiDamage = 0;
        }
        
        
        bool VerifyMobs()
        {
            bool HasRemainingAttacker = false;
            for(int i = 0; i < 4; i++)
            {
                if(guidMob[i])
                {
                    if (Unit* unit = sObjectAccessor->FindUnit(guidMob[i]))
                    {
                        if(unit->isAlive())
                            HasRemainingAttacker = true;
                    }
                    else
                        guidMob[i] = 0;
                }
            }

            return !HasRemainingAttacker;
        }
        
        void ResetMobs()
        {
            events.ScheduleEvent(EVENT_CHECK_MOBS, 1000);
            me->HandleEmoteCommand(EMOTE_STATE_READY2H);

            for(int i = 0; i < 4; i++)
            {
                if(guidMob[i])
                    if (Unit* unit = sObjectAccessor->FindUnit(guidMob[i]))
                        if (unit->ToCreature())
                            unit->ToCreature()->DespawnOrUnsummon();

                guidMob[i] = 0;

                if(TempSummon* temp = me->SummonCreature(54130, me->GetPositionX()-3+rand()%6, me->GetPositionY() + 4 + rand()%4, me->GetPositionZ()+2, 3.3f, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 10000))
                {
                    guidMob[i] = temp->GetGUID();
                    
                    temp->SetFacingToObject(me);
                    temp->HandleEmoteCommand(EMOTE_STATE_READY2H);
                    
                    temp->GetMotionMaster()->Clear(false);
                    temp->GetMotionMaster()->MoveChase(me);
                    temp->Attack(me, true);
                    temp->getThreatManager().addThreat(me, 250.0f);
                }
            }
        }
        
        void UpdateAI(const uint32 diff)
        {
            events.Update(diff);
            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_CHECK_MOBS:
                    {
                        if(VerifyMobs()) //plus de mobs, win!
                        {
                    	    me->HandleEmoteCommand(EMOTE_STATE_STAND);
                    	    me->MonsterYell("Thank you!", LANG_UNIVERSAL, 0);
                        
                            std::list<Player*> PlayerList;
                            GetPlayerListInGrid(PlayerList, me, 20.0f);
                            for (auto player: PlayerList)
                                player->KilledMonsterCredit(54855, 0);
                        
                            events.ScheduleEvent(EVENT_RESET, 30000);
                        }
                        else
                            events.ScheduleEvent(EVENT_CHECK_MOBS, 1000);

                        break;
                    }
                    case EVENT_RESET:
                    {
                        ResetMobs();
                    }
                }
            }
        }
    };
};

class mob_aysa_lake_escort : public CreatureScript
{
public:
    mob_aysa_lake_escort() : CreatureScript("mob_aysa_lake_escort") { }

    struct mob_aysa_lake_escortAI : public npc_escortAI
    {        
        mob_aysa_lake_escortAI(Creature* creature) : npc_escortAI(creature)
        {}

        uint32 IntroTimer;

        void Reset()
        {
            IntroTimer = 2500;
        }

        void MovementInform(uint32 uiType, uint32 uiId)
        {
            npc_escortAI::MovementInform(uiType, uiId);

            if (uiType != POINT_MOTION_TYPE && uiType != EFFECT_MOTION_TYPE)
                return;

            switch (uiId)
            {
                case 10:
                    me->GetMotionMaster()->MoveJump(1227.11f, 3489.73f, 100.37f, 10, 20, 11);
                    break;
                case 11:
                    me->GetMotionMaster()->MoveJump(1236.68f, 3456.68f, 102.58f, 10, 20, 12);
                    break;
                case 12:
                    Start(false, true);
                    break;
                default:
                    break;
            }
        }

        void WaypointReached(uint32 waypointId)
        {
            if (waypointId == 4)
                me->DespawnOrUnsummon(500);
        }

        void UpdateAI(const uint32 diff)
        {
            if (IntroTimer)
            {
                if (IntroTimer <= diff)
                {
                    me->MonsterYell("Follow me!", LANG_UNIVERSAL, 0);
                    IntroTimer = 0;
                    me->GetMotionMaster()->MoveJump(1216.78f, 3499.44f, 91.15f, 10, 20, 10);
                }
                else
                    IntroTimer -= diff;
            }

            npc_escortAI::UpdateAI(diff);
        }
    };
    
    CreatureAI* GetAI(Creature* creature) const
    {
        return new mob_aysa_lake_escortAI(creature);
    }
    
};

class mob_aysa : public CreatureScript
{
public:
    mob_aysa() : CreatureScript("mob_aysa") { }

    bool OnQuestAccept(Player* player, Creature* creature, Quest const* quest)
    {
        if (quest->GetQuestId() == 29414) // La voie des tushui
            if (Creature* tempSummon = creature->SummonCreature(56661, creature->GetPositionX(), creature->GetPositionY(), creature->GetPositionZ(), creature->GetOrientation(), TEMPSUMMON_MANUAL_DESPAWN, 0, player->GetGUID()))
                tempSummon->SetPhaseMask(1, true);

        return true;
    }
    
    CreatureAI* GetAI(Creature* creature) const
    {
        return new mob_aysaAI(creature);
    }
    
    struct mob_aysaAI : public ScriptedAI
    {
    	EventMap events;
        std::list<Player*> playersInvolved;
        TempSummon* lifei;
        bool inCombat;
        uint32 timer;
        
        mob_aysaAI(Creature* creature) : ScriptedAI(creature)
        {
            events.ScheduleEvent(1, 600); //Begin script
            inCombat = false;
            timer = 0;
            lifei = NULL;
            me->SetReactState(REACT_DEFENSIVE);
            me->setFaction(2263);
        }

        enum eEvents
        {
            EVENT_START         = 1,
            EVENT_SPAWN_MOBS    = 2,
            EVENT_PROGRESS      = 3,
            EVENT_END           = 4,
        };
        
        void DamageTaken(Unit* pDoneBy, uint32 &uiDamage)
        {
            if(me->HealthBelowPctDamaged(5, uiDamage))
            {
                if(lifei)
                {
                    lifei->UnSummon();
                    lifei = NULL;
                }
                
                uiDamage = 0;
                me->MonsterSay("I can't meditate!", LANG_UNIVERSAL, 0);
                me->SetFullHealth();
                me->SetReactState(REACT_DEFENSIVE);
                
                std::list<Creature*> unitlist;
                GetCreatureListWithEntryInGrid(unitlist, me, 59637, 50.0f);
                for (auto creature: unitlist)
                    me->Kill(creature);
                	
                events.ScheduleEvent(EVENT_START, 20000);
                events.CancelEvent(EVENT_SPAWN_MOBS);
                events.CancelEvent(EVENT_PROGRESS);
                events.CancelEvent(EVENT_END);
            }
        }
        
        void updatePlayerList()
        {
            playersInvolved.clear();
            
            std::list<Player*> PlayerList;
            GetPlayerListInGrid(PlayerList, me, 20.0f);

            for (auto player: PlayerList)
                if(player->GetQuestStatus(29414) == QUEST_STATUS_INCOMPLETE)
                    playersInvolved.push_back(player);
        }
        
        void UpdateAI(const uint32 diff)
        {
            events.Update(diff);
            while (uint32 eventId = events.ExecuteEvent())
            {
                switch(eventId)
                {
                    case EVENT_START: //Begin script if playersInvolved is not empty
                    {
                    	updatePlayerList();
                        if(playersInvolved.empty())
                            events.ScheduleEvent(1, 600);
                        else
                        {
                            me->MonsterSay("Keep those creatures at bay while I meditate. We'll soon have the answers we seek...", LANG_UNIVERSAL, 0);
                            me->SetReactState(REACT_PASSIVE);
                            timer = 0;
                            events.ScheduleEvent(EVENT_SPAWN_MOBS, 5000); //spawn mobs
                            events.ScheduleEvent(EVENT_PROGRESS, 1000); //update time
                            events.ScheduleEvent(EVENT_END, 90000); //end quest
                        }
                        break;
                    }
                    case EVENT_SPAWN_MOBS: //Spawn 3 mobs
                    {
                        updatePlayerList();
                        for(int i = 0; i < std::max((int)playersInvolved.size()*3,3); i++)
                        {
                            if(TempSummon* temp = me->SummonCreature(59637, 1144.55f, 3435.65f, 104.97f, 3.3f,TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 10000))
                            {
                                if (temp->AI())
                                    temp->AI()->AttackStart(me);

			                    temp->AddThreat(me, 250.0f);
                                temp->GetMotionMaster()->Clear();
                                temp->GetMotionMaster()->MoveChase(me);
                            }
                        }
                        events.ScheduleEvent(EVENT_SPAWN_MOBS, 20000); //spawn mobs
                        break;
                    }
                    case EVENT_PROGRESS: //update energy
                    {
                        timer++;
                        
                        if(timer == 25 && !lifei)
                        {
                            if (lifei = me->SummonCreature(54856, 1130.162231f, 3435.905518f, 105.496597f, 0.0f,TEMPSUMMON_MANUAL_DESPAWN))
                                lifei->MonsterSay("The way of the Tushui... enlightenment through patience and mediation... the principled life", LANG_UNIVERSAL, 0);
                        }
                        
                        if(timer == 30)
                            if (lifei)
                                lifei->MonsterSay("It is good to see you again, Aysa. You've come with respect, and so I shall give you the answers you seek.", LANG_UNIVERSAL, 0);
                        
                        if(timer == 42)
                            if (lifei)
                                lifei->MonsterSay("Huo, the spirit of fire, is known for his hunger. He wants for tinder to eat. He needs the caress of the wind to rouse him.", LANG_UNIVERSAL, 0);
                        
                        if(timer == 54)
                            if (lifei)
                                lifei->MonsterSay("If you find these things and bring them to his cave, on the far side of Wu-Song Village, you will face a challenge within.", LANG_UNIVERSAL, 0);
                        
                        if(timer == 66)
                            if (lifei)
                                lifei->MonsterSay("Overcome that challenge, and you shall be graced by Huo's presence. Rekindle his flame, and if your spirit is pure, he shall follow you.", LANG_UNIVERSAL, 0);
                        
                        if(timer == 78)
                            if (lifei)
                                lifei->MonsterSay("Go, children. We shall meet again very soon.", LANG_UNIVERSAL, 0);
                        
                        if(timer == 85)
                        {
                            if (lifei)
                                lifei->UnSummon();

                            lifei = NULL;
                        }
                        
                        updatePlayerList();
                        for (auto player: playersInvolved)
                        {
                            if(!player->HasAura(116421))
                                player->CastSpell(player, 116421);

                            player->ModifyPower(POWER_ALTERNATE_POWER, timer/25);
                            player->SetMaxPower(POWER_ALTERNATE_POWER, 90);
                        }

                        events.ScheduleEvent(EVENT_PROGRESS, 1000);
                        break;
                    }
                    case EVENT_END: //script end
                    {
                        if(lifei)
                        {
                            lifei->UnSummon();
                            lifei = NULL;
                        }
                        events.ScheduleEvent(EVENT_START, 10000);
                        events.CancelEvent(EVENT_SPAWN_MOBS);
                        events.CancelEvent(EVENT_PROGRESS);
                        me->MonsterSay("And so our path lays before us. Speak to Master Shang Xi, he will tell you what comes next.", LANG_UNIVERSAL, 0);
                        updatePlayerList();
                        me->SetReactState(REACT_DEFENSIVE);
                        for(auto player: playersInvolved)
                        {
                            player->KilledMonsterCredit(54856, 0);
                            player->RemoveAura(116421);
                        }
                        break;
                    }
                }
            }
        }
    };
};

class boss_living_air : public CreatureScript
{
public:
    boss_living_air() : CreatureScript("boss_living_air") { }
    
    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_living_airAI(creature);
    }
    
    struct boss_living_airAI : public ScriptedAI
    {
        boss_living_airAI(Creature* creature) : ScriptedAI(creature)
        {
            me->SetReactState(REACT_AGGRESSIVE);
        }
        
        EventMap events;
        
        void EnterCombat(Unit* unit)
        {
            events.ScheduleEvent(1, 3000);
            events.ScheduleEvent(2, 5000);
        }
        
        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;
            
            events.Update(diff);
            
            while (uint32 eventId = events.ExecuteEvent())
            {
                switch(eventId)
                {
                    case 1:
                    	me->CastSpell(me->getVictim(), 108693);
                    	break;
                    case 2:
                    	me->CastSpell(me->getVictim(), 73212);
                    	events.ScheduleEvent(2, 5000);
                    	break;
                }
            }
            
            DoMeleeAttackIfReady();
        }
    };
};

#define QUEST_PARCHEMIN_VOLANT  29421

class boss_li_fei : public CreatureScript
{
public:
    boss_li_fei() : CreatureScript("boss_li_fei") {}

    bool OnQuestAccept(Player* player, Creature* creature, Quest const* quest)
    {
        if (quest->GetQuestId() == QUEST_PARCHEMIN_VOLANT) // La lecon du parchemin brulant
        {
            if (Creature* tempSummon = creature->SummonCreature(54856, creature->GetPositionX(), creature->GetPositionY(), creature->GetPositionZ(), creature->GetOrientation(), TEMPSUMMON_MANUAL_DESPAWN, 0, player->GetGUID()))
            {
                tempSummon->SetPhaseMask(1024, true);
                tempSummon->AI()->AttackStart(player);
                tempSummon->AI()->SetGUID(player->GetGUID());
            }
        }

        return true;
    }
};

class boss_li_fei_fight : public CreatureScript
{
public:
    boss_li_fei_fight() : CreatureScript("boss_li_fei_fight") { }
    
    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_li_fei_fightAI(creature);
    }
    
    struct boss_li_fei_fightAI : public ScriptedAI
    {
        EventMap events;
        std::list<Player*> playersInvolved;
        uint64 playerGuid;

        boss_li_fei_fightAI(Creature* creature) : ScriptedAI(creature)
        {}

        enum eEvents
        {
            EVENT_CHECK_PLAYER      = 1,
            EVENT_FEET_OF_FURY      = 2,
            EVENT_SHADOW_KICK       = 3,
            EVENT_SHADOW_KICK_STUN  = 4,
        };

        void Reset()
        {
            // This particular entry is also spawned on an other event
            if (me->GetAreaId() != 5849) // Cavern areaid
                return;

            playerGuid = 0;
            me->SetReactState(REACT_AGGRESSIVE);
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_IMMUNE_TO_PC);
            me->setFaction(16);
            events.ScheduleEvent(EVENT_CHECK_PLAYER, 2500);
            events.ScheduleEvent(EVENT_FEET_OF_FURY, 5000);
            events.ScheduleEvent(EVENT_SHADOW_KICK,  1000);
        }

        void SetGUID(uint64 guid, int32 /*type*/)
        {
            playerGuid = guid;
        }
        
        void DamageTaken(Unit* attacker, uint32& damage)
        {
            if (me->HealthBelowPctDamaged(10, damage))
            {
                damage = 0;
                me->setFaction(35);
                me->CombatStop();
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC);
                me->HandleEmoteCommand(EMOTE_ONESHOT_SALUTE);

                if (Player* player = ObjectAccessor::FindPlayer(playerGuid))
                    player->KilledMonsterCredit(54734, 0);
            }
        }

        void KilledUnit(Unit* victim)
        {
            if (victim->GetTypeId() == TYPEID_PLAYER)
            {
                victim->ToPlayer()->SetQuestStatus(QUEST_PARCHEMIN_VOLANT, QUEST_STATUS_FAILED);

                if (victim->GetGUID() == playerGuid)
                    me->DespawnOrUnsummon(3000);
            }
        }
        
        void UpdateAI(const uint32 diff)
        {
            events.Update(diff);
            
            while (uint32 eventId = events.ExecuteEvent())
            {
                switch(eventId)
                {
                    case EVENT_CHECK_PLAYER:
                    {
                        bool checkPassed = true;
                        Player* player = ObjectAccessor::FindPlayer(playerGuid);

                        if (!player)
                        {
                            me->DespawnOrUnsummon(1000);
                            playerGuid = 0;
                            break;
                        }

                        if (!player->isAlive())
                        {
                            me->DespawnOrUnsummon(1000);
                            playerGuid = 0;
                            break;
                        }
                        
                        if (player->GetQuestStatus(QUEST_PARCHEMIN_VOLANT) != QUEST_STATUS_INCOMPLETE)
                        {
                            me->DespawnOrUnsummon(1000);
                            playerGuid = 0;
                            break;
                        }

                        events.ScheduleEvent(EVENT_CHECK_PLAYER, 2500);
                        break;
                    }
                    case EVENT_FEET_OF_FURY:
                        if(me->getVictim())
                    	    me->CastSpell(me->getVictim(), 108958);

                        events.ScheduleEvent(EVENT_FEET_OF_FURY, 5000);
                    	break;
                    case EVENT_SHADOW_KICK:
                        if(me->getVictim())
                    	    me->CastSpell(me->getVictim(), 108936);

                    	events.ScheduleEvent(EVENT_SHADOW_KICK_STUN, 2500);
                    	events.ScheduleEvent(EVENT_SHADOW_KICK, 30000);
                    	break;
                    case 4:
                        if(me->getVictim())
                    	    me->CastSpell(me->getVictim(), 108944);
                    	break;
                }
            }
            
            DoMeleeAttackIfReady();
        }
    };
};

// Huo Benediction - 102630
class spell_huo_benediction: public SpellScriptLoader
{
    public:
        spell_huo_benediction() : SpellScriptLoader("spell_huo_benediction") { }

        class spell_huo_benediction_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_huo_benediction_AuraScript);
            
            void OnApply(constAuraEffectPtr /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                Unit* target = GetTarget();

                if (!target)
                    return;

                std::list<Creature*> huoList;
                GetCreatureListWithEntryInGrid(huoList, target, 54958, 20.0f);

                for (auto huo: huoList)
                    if (huo->ToTempSummon())
                        if (huo->ToTempSummon()->GetOwnerGUID() == target->GetGUID())
                            return;

                // A partir d'ici on sait que le joueur n'a pas encore de Huo
                if (TempSummon* tempHuo = target->SummonCreature(54958, target->GetPositionX(), target->GetPositionY(), target->GetPositionZ(), 0.0f, TEMPSUMMON_MANUAL_DESPAWN, 0, target->GetGUID()))
                {
                    tempHuo->SetOwnerGUID(target->GetGUID());
                    tempHuo->GetMotionMaster()->MoveFollow(target, PET_FOLLOW_DIST, PET_FOLLOW_ANGLE);
                }
            }

            void OnRemove(constAuraEffectPtr /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                Unit* target = GetTarget();

                if (!target)
                    return;

                std::list<Creature*> huoList;
                GetCreatureListWithEntryInGrid(huoList, target, 54958, 20.0f);

                for (auto huo: huoList)
                    if (huo->ToTempSummon())
                        if (huo->ToTempSummon()->GetOwnerGUID() == target->GetGUID())
                            huo->DespawnOrUnsummon();
            }

            void Register()
            {
                OnEffectApply  += AuraEffectApplyFn (spell_huo_benediction_AuraScript::OnApply,  EFFECT_0, SPELL_AURA_MOD_INCREASE_SPEED, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
                OnEffectRemove += AuraEffectRemoveFn(spell_huo_benediction_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_MOD_INCREASE_SPEED, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_huo_benediction_AuraScript();
        }
};

class AreaTrigger_at_temple_entrance : public AreaTriggerScript
{
    public:
        AreaTrigger_at_temple_entrance() : AreaTriggerScript("AreaTrigger_at_temple_entrance")
        {}

        bool OnTrigger(Player* player, AreaTriggerEntry const* trigger)
        {
            if (player->GetQuestStatus(29423) == QUEST_STATUS_INCOMPLETE)
            {
                player->KilledMonsterCredit(61128, 0);

                std::list<Creature*> huoList;
                GetCreatureListWithEntryInGrid(huoList, player, 54958, 20.0f);

                for (auto huo: huoList)
                {
                    if (huo->ToTempSummon())
                    {
                        if (huo->ToTempSummon()->GetOwnerGUID() == player->GetGUID())
                        {
                            huo->GetMotionMaster()->Clear();
                            huo->GetMotionMaster()->MovePoint(1, 950.0f, 3601.0f, 203.0f);
                            huo->DespawnOrUnsummon(5000);
                        }
                    }
                }
            }

            return true;
        }
};

void AddSC_wandering_isle()
{
    new npc_jaomin_ro();
    new boss_living_air();
    new boss_li_fei();
    new boss_li_fei_fight();
    new boss_zhao_ren();
    new boss_vordraka();
    new mob_master_shang_xi();
    new go_wandering_weapon_rack();
    new mob_training_target();
    new mob_tushui_trainee();
    new mob_attacker_dimwind();
    new mob_min_dimwind();
    new mob_aysa_lake_escort();
    new mob_aysa();
    new spell_huo_benediction();
    new AreaTrigger_at_temple_entrance();
    new AreaTrigger_at_bassin_curse();
    new vehicle_balance_pole();
    new mob_tushui_monk();
    new spell_rock_jump();
    new mob_shu_water_spirit();
    new spell_shu_benediction();
    new spell_grab_carriage();
    new npc_nourished_yak();
    new npc_water_spirit_dailo();
    new mob_master_shang_xi_temple();
    new npc_wind_vehicle();
    new AreaTrigger_at_wind_temple_entrance();
    new mob_aysa_wind_temple_escort();
    new mob_frightened_wind();
    new npc_aysa_in_wind_temple();
    new npc_rocket_launcher();
    new mob_master_shang_xi_after_zhao();
    new mob_master_shang_xi_after_zhao_escort();
    new mob_master_shang_xi_thousand_staff();
    new mob_master_shang_xi_thousand_staff_escort();
    new spell_grab_air_balloon();
    new mob_shang_xi_air_balloon();
    new AreaTrigger_at_mandori();
    new mob_mandori_escort();
    new npc_korga();
    new mob_ji_forest_escort();
    new AreaTrigger_at_rescue_soldiers();
    new npc_hurted_soldier();
    new mob_aysa_gunship_crash();
    new mob_aysa_gunship_crash_escort();
    new npc_ji_end_event();
    new npc_shen_healer();
    new npc_shang_xi_choose_faction();
}

class AreaTrigger_at_bassin_curse : public AreaTriggerScript
{
    public:
        AreaTrigger_at_bassin_curse() : AreaTriggerScript("AreaTrigger_at_bassin_curse") { }

        enum eTriggers
        {
            AREA_CRANE              = 6991,
            AREA_SKUNK              = 6988,
            AREA_FROG               = 6987,
            AREA_FROG_EXIT          = 6986,
            AREA_TURTLE             = 7012,
            AREA_CROCODILE          = 6990
        };

        enum eSpells
        {
            SPELL_FROG              = 102938,
            SPELL_SKUNK             = 102939,
            SPELL_TURTLE            = 102940,
            SPELL_CRANE             = 102941,
            SPELL_CROCODILE         = 102942
        };

        void AddOrRemoveSpell(Player* player, uint32 spellId)
        {
            RemoveAllSpellsExcept(player, spellId);

            if (!player->HasAura(spellId))
            {
                if (!player->IsOnVehicle())
                    player->AddAura(spellId, player);
            }
            else
                player->RemoveAurasDueToSpell(spellId);
        }

        void RemoveAllSpellsExcept(Player* player, uint32 spellId)
        {
            uint32 spellTable[5] = {SPELL_FROG, SPELL_SKUNK, SPELL_TURTLE, SPELL_CRANE, SPELL_CROCODILE};

            for (uint8 i = 0; i < 5; ++i)
                if (spellId != spellTable[i])
                    player->RemoveAurasDueToSpell(spellTable[i]);
        }

        bool OnTrigger(Player* player, AreaTriggerEntry const* trigger)
        {
            switch(trigger->id)
            {
                case AREA_CRANE:     AddOrRemoveSpell(player, SPELL_CRANE);     break;
                case AREA_SKUNK:     AddOrRemoveSpell(player, SPELL_SKUNK);     break;
                case AREA_FROG:      AddOrRemoveSpell(player, SPELL_FROG);      break;
                case AREA_FROG_EXIT: RemoveAllSpellsExcept(player, 0);          break;
                case AREA_TURTLE:    AddOrRemoveSpell(player, SPELL_TURTLE);    break;
                case AREA_CROCODILE: AddOrRemoveSpell(player, SPELL_CROCODILE); break;
            }
            return true;
        }
};

// Npc's : 54993 - 55083 - 57431
class vehicle_balance_pole : public VehicleScript
{
    public:
        vehicle_balance_pole() : VehicleScript("vehicle_balance_pole") {}

        void OnAddPassenger(Vehicle* veh, Unit* passenger, int8 /*seatId*/)
        {
            if (passenger->HasAura(102938))
                passenger->ExitVehicle();
        }

        /*void OnRemovePassenger(Vehicle* veh, Unit* passenger)
        {
            if (veh->GetBase()->GetPositionZ() == 116.521004f) // Hack
                if (passenger->IsOnVehicle()) // Maybe the player
                    passenger->AddAura(102938, passenger);
        }*/
};

class mob_tushui_monk : public CreatureScript
{
public:
    mob_tushui_monk() : CreatureScript("mob_tushui_monk") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new mob_tushui_monkAI(creature);
    }

    struct mob_tushui_monkAI : public ScriptedAI
    {
        mob_tushui_monkAI(Creature* creature) : ScriptedAI(creature)
        {}

        void Reset()
        {
            std::list<Creature*> poleList;
            GetCreatureListWithEntryInGrid(poleList, me, 54993, 25.0f);

            if (poleList.empty())
            {
                me->DespawnOrUnsummon(1000);
                return;
            }

            JadeCore::Containers::RandomResizeList(poleList, 1);

            for (auto creature: poleList)
                me->EnterVehicle(creature);

            me->setFaction(2357);
        }

        void JustDied(Unit* /*killer*/)
        {
            me->ExitVehicle();
            me->DespawnOrUnsummon(1000);
        }
    };
};

// Rock Jump - 103069 / 103070 / 103077
class spell_rock_jump: public SpellScriptLoader
{
    public:
        spell_rock_jump() : SpellScriptLoader("spell_rock_jump") { }

        class spell_rock_jump_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_rock_jump_SpellScript);

            void HandleScriptEffect(SpellEffIndex effIndex)
            {
                if (Unit* caster = GetCaster())
                {
                    if (caster->GetPositionZ() < 90.0f)
                        caster->GetMotionMaster()->MoveJump(1045.36f, 2848.47f, 91.38f, 10.0f, 10.0f);
                    else if (caster->GetPositionZ() < 92.0f)
                        caster->GetMotionMaster()->MoveJump(1054.42f, 2842.65f, 92.96f, 10.0f, 10.0f);
                    else if (caster->GetPositionZ() < 94.0f)
                        caster->GetMotionMaster()->MoveJump(1063.66f, 2843.49f, 95.50f, 10.0f, 10.0f);
                    else
                    {
                        caster->GetMotionMaster()->MoveJump(1078.42f, 2845.07f, 95.16f, 10.0f, 10.0f);

                        if (caster->ToPlayer())
                            caster->ToPlayer()->KilledMonsterCredit(57476);
                    }
                }
            }

            void Register()
            {
                OnEffectLaunch += SpellEffectFn(spell_rock_jump_SpellScript::HandleScriptEffect, EFFECT_0, SPELL_EFFECT_JUMP_DEST);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_rock_jump_SpellScript();
        }
};

Position rocksPos[4] =
{
    {1102.05f, 2882.11f, 94.32f, 0.11f},
    {1120.01f, 2883.20f, 96.44f, 4.17f},
    {1128.09f, 2859.44f, 97.64f, 2.51f},
    {1111.52f, 2849.84f, 94.84f, 1.94f}
};

class mob_shu_water_spirit : public CreatureScript
{
public:
    mob_shu_water_spirit() : CreatureScript("mob_shu_water_spirit") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new mob_shu_water_spiritAI(creature);
    }

    struct mob_shu_water_spiritAI : public ScriptedAI
    {
        mob_shu_water_spiritAI(Creature* creature) : ScriptedAI(creature) {}

        EventMap _events;
        uint8 actualPlace;

        uint64 waterSpoutGUID;

        enum eShuSpells
        {
            SPELL_WATER_SPOUT_SUMMON    = 116810,
            SPELL_WATER_SPOUT_WARNING   = 116695,
            SPELL_WATER_SPOUT_EJECT     = 116696,
            SPELL_WATER_SPOUT_VISUAL    = 117057
        };

        enum eEvents
        {
            EVENT_CHANGE_PLACE          = 1,
            EVENT_SUMMON_WATER_SPOUT    = 2,
            EVENT_WATER_SPOUT_VISUAL    = 3,
            EVENT_WATER_SPOUT_EJECT     = 4,
            EVENT_WATER_SPOUT_DESPAWN   = 5
        };

        void Reset()
        {
            _events.Reset();
            actualPlace = 0;
            waterSpoutGUID = 0;

            _events.ScheduleEvent(EVENT_CHANGE_PLACE, 5000);
        }

        void MovementInform(uint32 typeId, uint32 pointId)
        {
            if (typeId != EFFECT_MOTION_TYPE)
                return;

            if (pointId == 1)
            {
                me->RemoveAurasDueToSpell(SPELL_WATER_SPOUT_WARNING);
                if (Player* player = me->SelectNearestPlayerNotGM(50.0f))
                {
                    me->SetOrientation(me->GetAngle(player));
                    me->SetFacingToObject(player);
                    _events.ScheduleEvent(EVENT_SUMMON_WATER_SPOUT, 2000);
                }
                else
                    _events.ScheduleEvent(EVENT_CHANGE_PLACE, 5000);
            }
        }

        Creature* getWaterSpout()
        {
            return me->GetMap()->GetCreature(waterSpoutGUID);
        }

        void UpdateAI(const uint32 diff)
        {
            _events.Update(diff);

            switch (_events.ExecuteEvent())
            {
                case EVENT_CHANGE_PLACE:
                {
                    uint8 newPlace = 0;

                    do
                    {
                        newPlace = urand(0, 3);
                    }
                    while (newPlace == actualPlace);

                    me->GetMotionMaster()->MoveJump(rocksPos[newPlace].GetPositionX(), rocksPos[newPlace].GetPositionY(), rocksPos[newPlace].GetPositionZ(), 10.0f, 10.0f, 1);
                    me->AddAura(SPELL_WATER_SPOUT_WARNING, me); // Just visual
                    actualPlace = newPlace;
                    break;
                }
                case EVENT_SUMMON_WATER_SPOUT:
                {
                    float x = 0.0f, y = 0.0f;
                    GetPositionWithDistInOrientation(me, 5.0f, me->GetOrientation() + frand(-M_PI, M_PI), x, y);
                    waterSpoutGUID = 0;

                    if (Creature* waterSpout = me->SummonCreature(60488, x, y, 92.189629f))
                        waterSpoutGUID = waterSpout->GetGUID();

                    _events.ScheduleEvent(EVENT_WATER_SPOUT_VISUAL, 500);
                    _events.ScheduleEvent(EVENT_WATER_SPOUT_EJECT, 7500);
                    break;
                }
                case EVENT_WATER_SPOUT_VISUAL:
                {
                    if (Creature* waterSpout = getWaterSpout())
                        waterSpout->CastSpell(waterSpout, SPELL_WATER_SPOUT_WARNING, true);
                    break;
                }
                case EVENT_WATER_SPOUT_EJECT:
                {
                    if (Creature* waterSpout = getWaterSpout())
                    {
                        std::list<Player*> playerList;
                        GetPlayerListInGrid(playerList, waterSpout, 1.0f);

                        for (auto player: playerList)
                            player->CastSpell(player, SPELL_WATER_SPOUT_EJECT, true);

                        waterSpout->CastSpell(waterSpout, SPELL_WATER_SPOUT_VISUAL, true);
                    }
                    _events.ScheduleEvent(EVENT_WATER_SPOUT_DESPAWN, 3000);
                    break;
                }
                case EVENT_WATER_SPOUT_DESPAWN:
                {
                    if (Creature* waterSpout = getWaterSpout())
                        waterSpout->DespawnOrUnsummon();

                    waterSpoutGUID = 0;

                    _events.ScheduleEvent(EVENT_CHANGE_PLACE, 2000);
                    break;
                }
            }
        }
    };
};

// Shu Benediction - 103245
class spell_shu_benediction: public SpellScriptLoader
{
    public:
        spell_shu_benediction() : SpellScriptLoader("spell_shu_benediction") { }

        class spell_shu_benediction_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_shu_benediction_AuraScript);
            
            void OnApply(constAuraEffectPtr /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                Unit* target = GetTarget();

                if (!target)
                    return;

                std::list<Creature*> shuList;
                GetCreatureListWithEntryInGrid(shuList, target, 55213, 20.0f);

                for (auto shu: shuList)
                    if (shu->ToTempSummon())
                        if (shu->ToTempSummon()->GetOwnerGUID() == target->GetGUID())
                            return;

                // A partir d'ici on sait que le joueur n'a pas encore de Huo
                if (TempSummon* tempShu = target->SummonCreature(55213, target->GetPositionX(), target->GetPositionY(), target->GetPositionZ(), 0.0f, TEMPSUMMON_MANUAL_DESPAWN, 0, target->GetGUID()))
                {
                    tempShu->SetOwnerGUID(target->GetGUID());
                    tempShu->GetMotionMaster()->MoveFollow(target, PET_FOLLOW_DIST, PET_FOLLOW_ANGLE);
                }
            }

            void OnRemove(constAuraEffectPtr /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                Unit* target = GetTarget();

                if (!target)
                    return;

                std::list<Creature*> shuList;
                GetCreatureListWithEntryInGrid(shuList, target, 55213, 20.0f);

                for (auto shu: shuList)
                    if (shu->ToTempSummon())
                        if (shu->ToTempSummon()->GetOwnerGUID() == target->GetGUID())
                            shu->DespawnOrUnsummon();
            }

            void Register()
            {
                OnEffectApply  += AuraEffectApplyFn (spell_shu_benediction_AuraScript::OnApply,  EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
                OnEffectRemove += AuraEffectRemoveFn(spell_shu_benediction_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_shu_benediction_AuraScript();
        }
};

// Grab Carriage - 115904
class spell_grab_carriage: public SpellScriptLoader
{
    public:
        spell_grab_carriage() : SpellScriptLoader("spell_grab_carriage") { }

        class spell_grab_carriage_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_grab_carriage_SpellScript);

            void HandleScriptEffect(SpellEffIndex effIndex)
            {
                Unit* caster = GetCaster();

                if (!caster)
                    return;

                Creature* carriage = NULL;
                Creature* yak      = NULL;
                
                if (caster->GetAreaId() == 5826) // Bassins chantants
                {
                    carriage = caster->SummonCreature(57208, 979.06f, 2863.87f, 87.88f, 4.7822f, TEMPSUMMON_MANUAL_DESPAWN, 0, caster->GetGUID());
                    yak      = caster->SummonCreature(57207, 979.37f, 2860.29f, 88.22f, 4.4759f, TEMPSUMMON_MANUAL_DESPAWN, 0, caster->GetGUID());
                }
                else if (caster->GetAreaId() == 5881) // Ferme Dai-Lo
                {
                    carriage = caster->SummonCreature(57208, 588.70f, 3165.63f, 88.86f, 4.4156f, TEMPSUMMON_MANUAL_DESPAWN, 0, caster->GetGUID());
                    yak      = caster->SummonCreature(59499, 587.61f, 3161.91f, 89.31f, 4.3633f, TEMPSUMMON_MANUAL_DESPAWN, 0, caster->GetGUID());
                }
                else if (caster->GetAreaId() == 5833) // Epave du Chercheciel
                {
                    carriage = caster->SummonCreature(57208, 264.37f, 3867.60f, 73.56f, 0.9948f, TEMPSUMMON_MANUAL_DESPAWN, 0, caster->GetGUID());
                    yak      = caster->SummonCreature(57743, 268.38f, 3872.36f, 74.50f, 0.8245f, TEMPSUMMON_MANUAL_DESPAWN, 0, caster->GetGUID());
                }

                if (!carriage || !yak)
                    return;

                //carriage->CastSpell(yak, 108627, true);
                carriage->GetMotionMaster()->MoveFollow(yak, 0.0f, M_PI);
                caster->EnterVehicle(carriage, 0);
            }

            void Register()
            {
                OnEffectLaunch += SpellEffectFn(spell_grab_carriage_SpellScript::HandleScriptEffect, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_grab_carriage_SpellScript();
        }
};

class npc_nourished_yak : public CreatureScript
{
public:
    npc_nourished_yak() : CreatureScript("npc_nourished_yak") { }

    struct npc_nourished_yakAI : public npc_escortAI
    {        
        npc_nourished_yakAI(Creature* creature) : npc_escortAI(creature)
        {}

        uint32 IntroTimer;
        uint8 waypointToEject;

        void Reset()
        {
            uint8 waypointToEject = 100;

            if (me->isSummon())
            {
                IntroTimer = 2500;

                // Bassins chantants -> Dai-Lo
                if (me->GetAreaId() == 5826)
                    waypointToEject = 24;
                // Dai-Lo -> Temple
                else if (me->GetAreaId() == 5881) // Ferme Dai-Lo
                    waypointToEject = 22;
                // Epave -> Temple
                else if (me->GetAreaId() == 5833) // Epave du Chercheciel
                    waypointToEject = 18;
            }
            else
                IntroTimer = 0;
        }

        void WaypointReached(uint32 waypointId)
        {
            if (waypointId == waypointToEject)
            {
                if (Creature* vehicle = GetClosestCreatureWithEntry(me, 57208, 50.0f))
                    if (vehicle->GetVehicleKit())
                        vehicle->GetVehicleKit()->RemoveAllPassengers();
            }
        }

        void UpdateAI(const uint32 diff)
        {
            if (IntroTimer)
            {
                if (IntroTimer <= diff)
                {
                    Start(false, true);
                    IntroTimer = 0;
                }
                else
                    IntroTimer -= diff;
            }

            npc_escortAI::UpdateAI(diff);
        }
    };
    
    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_nourished_yakAI(creature);
    }
    
};

class npc_water_spirit_dailo : public CreatureScript
{
public:
    npc_water_spirit_dailo() : CreatureScript("npc_water_spirit_dailo") { }

    bool OnGossipHello(Player* player, Creature* creature)
    {
        if (creature->isQuestGiver())
            player->PrepareQuestMenu(creature->GetGUID());

        if (player->GetQuestStatus(29774) == QUEST_STATUS_INCOMPLETE)
             player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Can you please help us to wake up Wugou ?", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);

        player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());

        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action)
    {
        player->PlayerTalkClass->ClearMenus();
        if (action == GOSSIP_ACTION_INFO_DEF+1)
        {
            player->CLOSE_GOSSIP_MENU();
            player->KilledMonsterCredit(55548);
            player->RemoveAurasDueToSpell(59073); // Remove Phase 2, first water spirit disapear

            if (Creature* shu = player->SummonCreature(55558, creature->GetPositionX(), creature->GetPositionY(), creature->GetPositionZ(), creature->GetOrientation(), TEMPSUMMON_MANUAL_DESPAWN, 0, player->GetGUID()))
            {
                if (shu->AI())
                {
                    shu->AI()->DoAction(0);
                    shu->AI()->SetGUID(player->GetGUID());
                }
            }
        }
        return true;
    }

    struct npc_water_spirit_dailoAI : public ScriptedAI
    {
        npc_water_spirit_dailoAI(Creature* creature) : ScriptedAI(creature) {}

        uint64 playerGuid;
        uint16 eventTimer;
        uint8  eventProgress;

        void Reset()
        {
            eventTimer      = 0;
            eventProgress   = 0;
            playerGuid      = 0;
        }

        void DoAction(const int32 actionId)
        {
            eventTimer = 2500;
        }

        void SetGUID(uint64 guid, int32 /*type*/)
        {
            playerGuid = guid;
        }

        void MovementInform(uint32 typeId, uint32 pointId)
        {
            if (typeId != POINT_MOTION_TYPE)
                return;

            switch (pointId)
            {
                case 1:
                    eventTimer = 250;
                    ++eventProgress;
                    break;
                case 2:
                    eventTimer = 250;
                    ++eventProgress;
                    break;
                case 3:
                    if (Creature* wugou = GetClosestCreatureWithEntry(me, 60916, 20.0f))
                        me->SetFacingToObject(wugou);
                    me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STATE_READYUNARMED);
                    eventTimer = 2000;
                    ++eventProgress;
                    break;
                default:
                    break;
            }
        }

        void UpdateAI(const uint32 diff)
        {
            if (eventTimer)
            {
                if (eventTimer <= diff)
                {
                    switch (eventProgress)
                    {
                        case 0:
                            me->GetMotionMaster()->MovePoint(1, 650.30f, 3127.16f, 89.62f);
                            eventTimer = 0;
                            break;
                        case 1:
                            me->GetMotionMaster()->MovePoint(2, 625.25f, 3127.88f, 87.95f);
                            eventTimer = 0;
                            break;
                        case 2:
                            me->GetMotionMaster()->MovePoint(3, 624.44f, 3142.94f, 87.75f);
                            eventTimer = 0;
                            break;
                        case 3:
                            if (Creature* wugou = GetClosestCreatureWithEntry(me, 60916, 20.0f))
                                wugou->CastSpell(wugou, 118027, false);
                            me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_ONESHOT_NONE);
                            eventTimer = 3000;
                            ++eventProgress;
                            break;
                        case 4:
                            eventTimer = 0;
                            if (Player* owner = ObjectAccessor::FindPlayer(playerGuid))
                            {
                                owner->KilledMonsterCredit(55547);
                                owner->RemoveAurasDueToSpell(59074); // Remove phase 4, asleep wugou disappear
                                
                                if (Creature* wugou = GetClosestCreatureWithEntry(me, 60916, 20.0f))
                                    if (Creature* newWugou = owner->SummonCreature(60916, wugou->GetPositionX(), wugou->GetPositionY(), wugou->GetPositionZ(), wugou->GetOrientation(), TEMPSUMMON_MANUAL_DESPAWN, 0, owner->GetGUID()))
                                        newWugou->GetMotionMaster()->MoveFollow(owner, PET_FOLLOW_DIST, PET_FOLLOW_ANGLE);
                            
                                me->GetMotionMaster()->MoveFollow(owner, PET_FOLLOW_DIST, -PET_FOLLOW_ANGLE);
                            }
                            break;
                        default:
                            break;
                    }
                }
                else
                    eventTimer -= diff;
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_water_spirit_dailoAI(creature);
    }
};

class AreaTrigger_at_middle_temple_from_east : public AreaTriggerScript
{
    public:
        AreaTrigger_at_middle_temple_from_east() : AreaTriggerScript("AreaTrigger_at_middle_temple_from_east") {}

        bool OnTrigger(Player* player, AreaTriggerEntry const* trigger)
        {
            if (Creature* shu = GetClosestCreatureWithEntry(player, 55558, 25.0f))
                shu->DespawnOrUnsummon();

            if (Creature* wugou = GetClosestCreatureWithEntry(player, 60916, 25.0f))
                wugou->DespawnOrUnsummon();

            return true;
        }
};

#define GOSSIP_WIND     "I would like to go back on the top of the temple"

class mob_master_shang_xi_temple : public CreatureScript
{
    public:
        mob_master_shang_xi_temple() : CreatureScript("mob_master_shang_xi_temple") { }

        bool OnQuestAccept(Player* player, Creature* creature, Quest const* quest)
        {
            if (quest->GetQuestId() == 29776) // Brise du matin
            {
                if (Creature* vehicle = player->SummonCreature(55685, player->GetPositionX(), player->GetPositionY(), player->GetPositionZ(), player->GetOrientation()))
                {
                    player->AddAura(99385, vehicle);
                    player->EnterVehicle(vehicle);
                }
            }

            return true;
        }

        bool OnGossipHello(Player* player, Creature* creature)
        {
            if (creature->isQuestGiver())
                player->PrepareQuestMenu(creature->GetGUID());

            if (player->GetQuestStatus(29776) != QUEST_STATUS_NONE)
                    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_WIND, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);

            player->PlayerTalkClass->SendGossipMenu(1, creature->GetGUID());
            return true;
        }

        bool OnGossipSelect(Player* player, Creature* /*creature*/, uint32 /*sender*/, uint32 action)
        {
            if (action == GOSSIP_ACTION_INFO_DEF + 1)
            {
                /* The vehicle bug for now on TaranZhu, too much lags
                 *if (Creature* vehicle = player->SummonCreature(55685, player->GetPositionX(), player->GetPositionY(), player->GetPositionZ(), player->GetOrientation()))
                {
                    player->AddAura(99385, vehicle);
                    player->EnterVehicle(vehicle);
                }*/

                player->NearTeleportTo(926.58f, 3605.33f, 251.63f, 3.114f);
            }

            player->PlayerTalkClass->SendCloseGossip();
            return true;
        }
};

class npc_wind_vehicle : public CreatureScript
{
public:
    npc_wind_vehicle() : CreatureScript("npc_wind_vehicle") { }

    struct npc_wind_vehicleAI : public npc_escortAI
    {        
        npc_wind_vehicleAI(Creature* creature) : npc_escortAI(creature)
        {}

        uint32 IntroTimer;

        void Reset()
        {
            IntroTimer = 100;
        }

        void WaypointReached(uint32 waypointId)
        {
            if (waypointId == 6)
            {
                if (me->GetVehicleKit())
                    me->GetVehicleKit()->RemoveAllPassengers();

                me->DespawnOrUnsummon();
            }
        }

        void UpdateAI(const uint32 diff)
        {
            if (IntroTimer)
            {
                if (IntroTimer <= diff)
                {
                    Start(false, true);
                    IntroTimer = 0;
                }
                else
                    IntroTimer -= diff;
            }

            npc_escortAI::UpdateAI(diff);
        }
    };
    
    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_wind_vehicleAI(creature);
    }
    
};

class AreaTrigger_at_wind_temple_entrance : public AreaTriggerScript
{
    public:
        AreaTrigger_at_wind_temple_entrance() : AreaTriggerScript("AreaTrigger_at_wind_temple_entrance")
        {}

        bool OnTrigger(Player* player, AreaTriggerEntry const* trigger)
        {
            if (player->GetQuestStatus(29785) == QUEST_STATUS_INCOMPLETE)
            {
                if (Creature* aysa = player->SummonCreature(55744, 665.60f, 4220.66f, 201.93f, 1.93f, TEMPSUMMON_MANUAL_DESPAWN, 0, player->GetGUID()))
                    aysa->AI()->SetGUID(player->GetGUID());
            }

            return true;
        }
};

class mob_aysa_wind_temple_escort : public CreatureScript
{
    public:
        mob_aysa_wind_temple_escort() : CreatureScript("mob_aysa_wind_temple_escort") { }

    struct mob_aysa_wind_temple_escortAI : public npc_escortAI
    {        
        mob_aysa_wind_temple_escortAI(Creature* creature) : npc_escortAI(creature)
        {}
        
        uint32 IntroTimer;

        uint64 playerGuid;

        void Reset()
        {
            IntroTimer = 100;
            me->SetReactState(REACT_PASSIVE);
        }

        void SetGUID(uint64 guid, int32)
        {
            playerGuid = guid;
        }

        void DoAction(int32 const /*param*/)
        {
            SetEscortPaused(false);
        }

        void WaypointReached(uint32 waypointId)
        {
            switch (waypointId)
            {
                case 1:
                    SetEscortPaused(true);
                    me->SetFacingTo(2.38f);
                    break;
                case 6:
                    SetEscortPaused(true);
                    break;
                case 8:
                    if (Player* player = ObjectAccessor::GetPlayer(*me, playerGuid))
                        player->KilledMonsterCredit(55666);
                    break;
                default:
                    break;
            }
        }

        void UpdateAI(const uint32 diff)
        {
            if (IntroTimer)
            {
                if (IntroTimer <= diff)
                {
                    Start(false, true);
                    IntroTimer = 0;
                }
                else
                    IntroTimer -= diff;
            }

            npc_escortAI::UpdateAI(diff);
        }
    };
    
    CreatureAI* GetAI(Creature* creature) const
    {
        return new mob_aysa_wind_temple_escortAI(creature);
    }
};

class mob_frightened_wind : public CreatureScript
{
public:
    mob_frightened_wind() : CreatureScript("mob_frightened_wind") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new mob_frightened_windAI(creature);
    }

    struct mob_frightened_windAI : public ScriptedAI
    {
        mob_frightened_windAI(Creature* creature) : ScriptedAI(creature)
        {}

        uint32 tornadeTimer;

        enum Spells
        {
            SPELL_TORNADE    = 107278,
        };

        void Reset()
        {
            me->SetReactState(REACT_PASSIVE);
            tornadeTimer = 8 * IN_MILLISECONDS;
        }

        void UpdateAI(const uint32 diff)
        {
            if (tornadeTimer <= diff)
            {
                me->ToggleAura(SPELL_TORNADE, me);

                if (!me->HasAura(SPELL_TORNADE))
                {
                    std::list<Creature*> aysaList;
                    GetCreatureListWithEntryInGrid(aysaList, me, 55744, 35.0f);

                    for (auto aysa: aysaList)
                        aysa->AI()->DoAction(1);
                }
                tornadeTimer = 8 * IN_MILLISECONDS;
            }
            else
                tornadeTimer -= diff;
        }
    };
};

class npc_aysa_in_wind_temple : public CreatureScript
{
    public:
        npc_aysa_in_wind_temple() : CreatureScript("npc_aysa_in_wind_temple") { }

        bool OnQuestAccept(Player* player, Creature* creature, Quest const* quest)
        {
            if (quest->GetQuestId() == 29786) // Bataille Pyrotechnique
            {
                if (Creature* aysa = player->SummonCreature(64543, 543.94f, 4317.31f, 212.24f, 1.675520f, TEMPSUMMON_TIMED_DESPAWN, 10000, player->GetGUID()))
                    aysa->GetMotionMaster()->MovePoint(1, 643.45f, 4228.66f, 202.90f);
                
                if (Creature* dafeng = player->SummonCreature(64532, 543.56f, 4320.97f, 212.24f, 5.445430f, TEMPSUMMON_TIMED_DESPAWN, 10000, player->GetGUID()))
                    dafeng->GetMotionMaster()->MovePoint(1, 643.45f, 4228.66f, 202.90f);
            }

            return true;
        }
};

enum Enums
{
    NPC_ROCKET_LAUNCHER = 64507,
    SPELL_ROCKET_LAUNCH = 104855,
            
    EVENT_NEXT_MOVEMENT = 1,
    EVENT_STUNNED       = 2,
    EVENT_LIGHTNING     = 3,

    SPELL_SERPENT_SWEEP = 125990,
    SPELL_STUNNED       = 125992,
    SPELL_LIGHTNING     = 126006,
};

Position ZhaoPos[] = 
{
    {719.36f, 4164.60f, 216.06f}, // Center
    {745.91f, 4154.35f, 223.48f},
    {717.04f, 4141.16f, 219.83f},
    {689.62f, 4153.16f, 217.63f},
    {684.53f, 4173.24f, 216.98f},
    {704.77f, 4190.16f, 218.24f},
    {736.90f, 4183.85f, 221.41f}
};

class boss_zhao_ren : public CreatureScript
{
public:
    boss_zhao_ren() : CreatureScript("boss_zhao_ren") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_zhao_renAI(creature);
    }

    struct boss_zhao_renAI : public ScriptedAI
    {
        boss_zhao_renAI(Creature* creature) : ScriptedAI(creature)
        {}

        EventMap _events;
        bool eventStarted;
        uint8 hitCount;
        uint8 currentPos;

        void Reset()
        {
            _events.Reset();
            me->SetReactState(REACT_PASSIVE);

            eventStarted = false;
            hitCount = 0;
            currentPos = 0;

            me->SetFullHealth();
            me->RemoveAurasDueToSpell(SPELL_STUNNED);

            me->GetMotionMaster()->Clear();
            me->GetMotionMaster()->MovePoint(0, ZhaoPos[0].GetPositionX(), ZhaoPos[0].GetPositionY(), ZhaoPos[0].GetPositionZ());
        }

        void SpellHit(Unit* caster, const SpellInfo* spell)
        {
            if (spell->Id == SPELL_ROCKET_LAUNCH)
            {
                if (++hitCount >= 5)
                {
                    me->CastSpell(me, SPELL_STUNNED, true);
                    me->GetMotionMaster()->Clear();
                    me->GetMotionMaster()->MoveFall();
                    _events.ScheduleEvent(EVENT_STUNNED, 12000);
                    hitCount = 0;
                }
            }
        }
        
        bool checkPlayers()
        {
            std::list<Player*> playerList;
            GetPlayerListInGrid(playerList, me, 80.0f);

            for (auto player : playerList)
                if (player->GetQuestStatus(29786) == QUEST_STATUS_INCOMPLETE)
                    if (player->isAlive())
                        return true;

            return false;
        }

        void GoToNextPos()
        {
            if (++currentPos > 6)
                currentPos = 1;

            me->GetMotionMaster()->MovePoint(currentPos, ZhaoPos[currentPos].GetPositionX(), ZhaoPos[currentPos].GetPositionY(), ZhaoPos[currentPos].GetPositionZ());
        }

        Player* GetRandomPlayer()
        {
            std::list<Player*> playerList;
            GetPlayerListInGrid(playerList, me, 50.0f);

            if (playerList.empty())
                return NULL;

            JadeCore::Containers::RandomResizeList(playerList, 1);

            return *playerList.begin();
        }

        void MovementInform(uint32 type, uint32 id)
        {
            if (type != POINT_MOTION_TYPE)
                return;

            if (!id)
                return;

            _events.ScheduleEvent(EVENT_NEXT_MOVEMENT, 200);
        }

        void JustDied(Unit* attacker)
        {
            std::list<Player*> playerList;
            GetPlayerListInGrid(playerList, me, 50.0f);

            for (auto player : playerList)
                if (player->GetQuestStatus(29786) == QUEST_STATUS_INCOMPLETE)
                    if (player->isAlive())
                        player->KilledMonsterCredit(me->GetEntry());
        }

        void UpdateAI(const uint32 diff)
        {
            if (checkPlayers())
            {
                if (!eventStarted)  // Event not started, player found
                {
                    _events.ScheduleEvent(EVENT_NEXT_MOVEMENT, 1000);
                    _events.ScheduleEvent(EVENT_LIGHTNING, 5000);
                    eventStarted = true;
                }
            }
            else
            {
                if (eventStarted)  // Event started, no player found
                    Reset();

                return;
            }

            _events.Update(diff);

            switch (_events.ExecuteEvent())
            {
                case EVENT_NEXT_MOVEMENT:
                {
                    if (me->HasAura(SPELL_STUNNED))
                        _events.ScheduleEvent(EVENT_NEXT_MOVEMENT, 2000);

                    GoToNextPos();
                    break;
                }
                case EVENT_STUNNED:
                {
                    me->RemoveAurasDueToSpell(SPELL_STUNNED);
                    me->CastSpell(me, SPELL_SERPENT_SWEEP, false);
                    _events.ScheduleEvent(EVENT_NEXT_MOVEMENT, 3000);
                    break;
                }
                case EVENT_LIGHTNING:
                {
                    if (Player* player = GetRandomPlayer())
                        me->CastSpell(player, SPELL_LIGHTNING, false);

                    _events.ScheduleEvent(EVENT_LIGHTNING, 5000);
                    break;
                }
            }
        }
    };
};

class npc_rocket_launcher : public CreatureScript
{
public:
    npc_rocket_launcher() : CreatureScript("npc_rocket_launcher") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_rocket_launcherAI (creature);
    }

    struct npc_rocket_launcherAI : public ScriptedAI
    {
        npc_rocket_launcherAI(Creature* creature) : ScriptedAI(creature) {}

        uint32 cooldown;

        void Reset()
        {
            cooldown = 0;
            me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
        }

        void OnSpellClick(Unit* Clicker)
        {
            if (cooldown)
                return;

            if (Creature* zhao = GetClosestCreatureWithEntry(me, 55786, 50.0f))
                me->CastSpell(zhao, SPELL_ROCKET_LAUNCH, false);

            cooldown = 5000;
            me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
        }

        void EnterCombat(Unit* /*who*/)
        {
            return;
        }

        void UpdateAI(const uint32 diff)
        {
            if (cooldown)
            {
                if (cooldown <= diff)
                {
                    me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
                    cooldown = 0;
                }
                else
                    cooldown -= diff;
            }
        }
    };
};

class mob_master_shang_xi_after_zhao : public CreatureScript
{
    public:
        mob_master_shang_xi_after_zhao() : CreatureScript("mob_master_shang_xi_after_zhao") { }

        bool OnQuestAccept(Player* player, Creature* creature, Quest const* quest)
        {
            if (quest->GetQuestId() == 29787) // Digne de passer
                if (Creature* master = player->SummonCreature(56159, creature->GetPositionX(), creature->GetPositionY(), creature->GetPositionZ(), creature->GetOrientation(), TEMPSUMMON_MANUAL_DESPAWN, 0, player->GetGUID()))
                    master->AI()->SetGUID(player->GetGUID());

            return true;
        }
};

class mob_master_shang_xi_after_zhao_escort : public CreatureScript
{
    public:
        mob_master_shang_xi_after_zhao_escort() : CreatureScript("mob_master_shang_xi_after_zhao_escort") { }

    struct mob_master_shang_xi_after_zhao_escortAI : public npc_escortAI
    {        
        mob_master_shang_xi_after_zhao_escortAI(Creature* creature) : npc_escortAI(creature)
        {}
        
        uint32 IntroTimer;

        uint64 playerGuid;

        void Reset()
        {
            IntroTimer = 250;
            me->SetReactState(REACT_PASSIVE);
        }

        void SetGUID(uint64 guid, int32)
        {
            playerGuid = guid;
        }

        void WaypointReached(uint32 waypointId)
        {
            switch (waypointId)
            {
                case 6:
                    me->SummonCreature(56274, 845.89f, 4372.62f, 223.98f, 4.78f, TEMPSUMMON_CORPSE_DESPAWN, 0, playerGuid);
                    break;
                case 12:
                    me->SetFacingTo(0.0f);
                    SetEscortPaused(true);
                    break;
                case 17:
                    me->SetFacingTo(4.537860f);
                    me->DespawnOrUnsummon(1000);

                    if (Player* owner = ObjectAccessor::GetPlayer(*me, playerGuid))
                        owner->AddAura(59074, owner);
                    break;
                default:
                    break;
            }
        }

        void SummonedCreatureDespawn(Creature* summon)
        {
            if (summon->GetEntry() == 56274)
                SetEscortPaused(false);
        }

        void UpdateAI(const uint32 diff)
        {
            if (IntroTimer)
            {
                if (IntroTimer <= diff)
                {
                    Start(false, true);
                    IntroTimer = 0;
                }
                else
                    IntroTimer -= diff;
            }

            npc_escortAI::UpdateAI(diff);
        }
    };
    
    CreatureAI* GetAI(Creature* creature) const
    {
        return new mob_master_shang_xi_after_zhao_escortAI(creature);
    }
};

class mob_master_shang_xi_thousand_staff : public CreatureScript
{
    public:
        mob_master_shang_xi_thousand_staff() : CreatureScript("mob_master_shang_xi_thousand_staff") { }

        bool OnQuestAccept(Player* player, Creature* creature, Quest const* quest)
        {
            if (quest->GetQuestId() == 29790) // Digne de passer
                if (Creature* master = player->SummonCreature(56686, creature->GetPositionX(), creature->GetPositionY(), creature->GetPositionZ(), creature->GetOrientation(), TEMPSUMMON_MANUAL_DESPAWN, 0, player->GetGUID()))
                    master->AI()->SetGUID(player->GetGUID());

            return true;
        }
};

class mob_master_shang_xi_thousand_staff_escort : public CreatureScript
{
    public:
        mob_master_shang_xi_thousand_staff_escort() : CreatureScript("mob_master_shang_xi_thousand_staff_escort") { }

    struct mob_master_shang_xi_thousand_staff_escortAI : public npc_escortAI
    {        
        mob_master_shang_xi_thousand_staff_escortAI(Creature* creature) : npc_escortAI(creature)
        {}
        
        uint32 IntroTimer;
        uint32 DespawnTimer;

        uint64 playerGuid;

        void Reset()
        {
            IntroTimer = 250;
            DespawnTimer = 0;
            me->SetReactState(REACT_PASSIVE);
            SetRun(false);
        }

        void SetGUID(uint64 guid, int32)
        {
            playerGuid = guid;
        }

        void WaypointReached(uint32 waypointId)
        {
            switch (waypointId)
            {
                case 4:
                    SetEscortPaused(true);
                    me->SetFacingTo(4.522332f);
                    DespawnTimer = 3000;
                    break;
                default:
                    break;
            }
        }

        void UpdateAI(const uint32 diff)
        {
            if (IntroTimer)
            {
                if (IntroTimer <= diff)
                {
                    Start(false, true);
                    IntroTimer = 0;
                }
                else
                    IntroTimer -= diff;
            }

            if (DespawnTimer)
            {
                if (DespawnTimer <= diff)
                {
                    me->DespawnOrUnsummon();
                    me->SummonCreature(57874, 873.09f, 4462.25f, 241.27f, 3.80f, TEMPSUMMON_MANUAL_DESPAWN, 0, playerGuid);

                    if (Player* owner = ObjectAccessor::GetPlayer(*me, playerGuid))
                        owner->KilledMonsterCredit(56688);

                    DespawnTimer = 0;
                }
                else
                    DespawnTimer -= diff;
            }

            npc_escortAI::UpdateAI(diff);
        }
    };
    
    CreatureAI* GetAI(Creature* creature) const
    {
        return new mob_master_shang_xi_thousand_staff_escortAI(creature);
    }
};

// Grab Air Balloon - 95247
class spell_grab_air_balloon: public SpellScriptLoader
{
    public:
        spell_grab_air_balloon() : SpellScriptLoader("spell_grab_air_balloon") { }

        class spell_grab_air_balloon_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_grab_air_balloon_SpellScript);

            void HandleScriptEffect(SpellEffIndex effIndex)
            {
                PreventHitAura();

                if (Unit* caster = GetCaster())
                    if (Creature* balloon = caster->SummonCreature(55649, 915.55f, 4563.66f, 230.68f, 2.298090f, TEMPSUMMON_MANUAL_DESPAWN, 0, caster->GetGUID()))
                        caster->EnterVehicle(balloon, 0);
            }

            void Register()
            {
                OnEffectLaunch += SpellEffectFn(spell_grab_air_balloon_SpellScript::HandleScriptEffect, EFFECT_0, SPELL_EFFECT_APPLY_AURA);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_grab_air_balloon_SpellScript();
        }
};

class mob_shang_xi_air_balloon : public VehicleScript
{
    public:
        mob_shang_xi_air_balloon() : VehicleScript("mob_shang_xi_air_balloon") { }

    void OnAddPassenger(Vehicle* /*veh*/, Unit* passenger, int8 seatId)
    {
        if (seatId == 0)
            if (Player* player = passenger->ToPlayer())
                player->KilledMonsterCredit(56378);
    }

    struct mob_shang_xi_air_balloonAI : public npc_escortAI
    {        
        mob_shang_xi_air_balloonAI(Creature* creature) : npc_escortAI(creature)
        {}
        
        uint32 IntroTimer;

        void Reset()
        {
            IntroTimer = 250;
            me->setActive(true);
            me->SetReactState(REACT_PASSIVE);
        }

        void WaypointReached(uint32 waypointId)
        {
            switch (waypointId)
            {
                case 19:
                    if (me->GetVehicleKit())
                    {
                        if (Unit* passenger = me->GetVehicleKit()->GetPassenger(0))
                            if (Player* player = passenger->ToPlayer())
                            {
                                player->KilledMonsterCredit(55939);
                                player->AddAura(50550, player);
                            }

                        me->GetVehicleKit()->RemoveAllPassengers();
                    }

                    me->DespawnOrUnsummon();
                    break;
                default:
                    break;
            }
        }

        void UpdateAI(const uint32 diff)
        {
            if (IntroTimer)
            {
                if (IntroTimer <= diff)
                {
                    Start(false, true);
                    IntroTimer = 0;
                }
                else
                    IntroTimer -= diff;
            }

            npc_escortAI::UpdateAI(diff);
        }
    };
    
    CreatureAI* GetAI(Creature* creature) const
    {
        return new mob_shang_xi_air_balloonAI(creature);
    }
};

class AreaTrigger_at_mandori : public AreaTriggerScript
{
    public:
        AreaTrigger_at_mandori() : AreaTriggerScript("AreaTrigger_at_mandori")
        {}

        bool OnTrigger(Player* player, AreaTriggerEntry const* trigger)
        {
           if (player->GetPositionX() < 710.0f)
               return true;

           if (player->GetQuestStatus(29792) != QUEST_STATUS_INCOMPLETE)
               return true;

           uint64 playerGuid = player->GetGUID();

            Creature* Aysa = player->SummonCreature(59986, 698.04f, 3601.79f, 142.82f, 3.254830f, TEMPSUMMON_MANUAL_DESPAWN, 0, playerGuid); // Aysa
            Creature* Ji   = player->SummonCreature(59988, 698.06f, 3599.34f, 142.62f, 2.668790f, TEMPSUMMON_MANUAL_DESPAWN, 0, playerGuid); // Ji
            Creature* Jojo = player->SummonCreature(59989, 702.78f, 3603.58f, 142.01f, 3.433610f, TEMPSUMMON_MANUAL_DESPAWN, 0, playerGuid); // Jojo

            if (!Aysa || !Ji || !Jojo)
                return true;
            
            Aysa->AI()->SetGUID(playerGuid);
              Ji->AI()->SetGUID(playerGuid);
            Jojo->AI()->SetGUID(playerGuid);

            player->RemoveAurasDueToSpell(59073);
            player->RemoveAurasDueToSpell(59074);

            return true;
        }
};

class mob_mandori_escort : public CreatureScript
{
    public:
        mob_mandori_escort() : CreatureScript("mob_mandori_escort") { }

    struct mob_mandori_escortAI : public npc_escortAI
    {        
        mob_mandori_escortAI(Creature* creature) : npc_escortAI(creature)
        {}

        enum escortEntry
        {
            NPC_AYSA    = 59986,
            NPC_JI      = 59988,
            NPC_JOJO    = 59989
        };
        
        uint32 IntroTimer;
        uint32 doorEventTimer;

        uint8  IntroState;
        uint8  doorEventState;

        uint64 playerGuid;
        
        uint64 mandoriDoorGuid;
        uint64 peiwuDoorGuid;

        void Reset()
        {
            IntroTimer      = 250;
            doorEventTimer  = 0;

            IntroState      = 0;
            doorEventState  = 0;

            playerGuid      = 0;
            mandoriDoorGuid = 0;
            peiwuDoorGuid   = 0;

            me->SetReactState(REACT_PASSIVE);
        }

        void SetGUID(uint64 guid, int32 type)
        {
            playerGuid = guid;

            if (!Is(NPC_AYSA))
                return;

            if (GameObject* mandoriDoor = me->SummonGameObject(211294, 695.26f, 3600.99f, 142.38f, 3.04f, 0.0f, 0.0f, 0.0f, 0.0f, RESPAWN_IMMEDIATELY, playerGuid))
                mandoriDoorGuid = mandoriDoor->GetGUID();

            if (GameObject* peiwuDoor = me->SummonGameObject(211298, 566.52f, 3583.46f, 92.16f, 3.14f, 0.0f, 0.0f, 0.0f, 0.0f, RESPAWN_IMMEDIATELY, playerGuid))
                peiwuDoorGuid = peiwuDoor->GetGUID();
        }

        bool Is(uint32 npc_entry)
        {
            return me->GetEntry() == npc_entry;
        }

        void WaypointReached(uint32 waypointId)
        {
            switch (waypointId)
            {
                case 5:
                    SetEscortPaused(true);

                    // Jojo reach the waypoint 1 sec after the others
                    if (!Is(NPC_JOJO))
                        doorEventTimer = 2000;
                    else
                        doorEventTimer = 1000;
                    break;
                default:
                    break;
            }
        }

        void LastWaypointReached()
        {
            if (Is(NPC_JI))
                if (Player* player = ObjectAccessor::FindPlayer(playerGuid))
                    player->AddAura(68482, player); // Phase 8192
            
            if (Is(NPC_AYSA))
            {
                if (GameObject* mandoriDoor = me->GetMap()->GetGameObject(mandoriDoorGuid))
                    mandoriDoor->Delete();
                if (GameObject* peiwuDoor = me->GetMap()->GetGameObject(peiwuDoorGuid))
                    peiwuDoor->Delete();
            }
        }

        void UpdateAI(const uint32 diff)
        {
            if (IntroTimer)
            {
                if (IntroTimer <= diff)
                {
                    switch (++IntroState)
                    {
                        case 1:
                            if (Is(NPC_AYSA))
                                me->MonsterYell("Let's go !", LANG_UNIVERSAL, playerGuid);
                            IntroTimer = 1000;
                            break;
                        case 2:
                            if (Is(NPC_AYSA))
                            {
                                if (GameObject* mandoriDoor = me->GetMap()->GetGameObject(mandoriDoorGuid))
                                    mandoriDoor->SetGoState(GO_STATE_ACTIVE);

                                if (Player* player = ObjectAccessor::FindPlayer(playerGuid))
                                    player->KilledMonsterCredit(59946);
                            }
                            IntroTimer = 1000;
                            break;
                        case 3:
                            Start(false, true);
                            IntroTimer = 0;
                            break;
                    }
                }
                else
                    IntroTimer -= diff;
            }

            if (doorEventTimer)
            {
                if (doorEventTimer <= diff)
                {
                    switch (++doorEventState)
                    {
                        case 1:
                            if (Is(NPC_AYSA))
                                me->MonsterSay("The door is blocked!", LANG_UNIVERSAL, playerGuid);
                            doorEventTimer = 2500;
                            break;
                        case 2:
                            if (Is(NPC_JI))
                                me->MonsterSay("They blocked it with a rock on the other side, I can't open it!", LANG_UNIVERSAL, playerGuid);
                            doorEventTimer = 4000;
                            break;
                        case 3:
                            if (Is(NPC_JOJO))
                                me->GetMotionMaster()->MoveCharge(567.99f, 3583.41f, 94.74f);
                            doorEventTimer = 150;
                            break;
                        case 4:
                            if (Is(NPC_AYSA))
                                if (GameObject* peiwuDoor = me->GetMap()->GetGameObject(peiwuDoorGuid))
                                    peiwuDoor->SetGoState(GO_STATE_ACTIVE);
                            doorEventTimer = 2000;
                            break;
                       case 5:
                            if (Is(NPC_AYSA))
                            {
                                me->MonsterSay("Well done, Jojo!", LANG_UNIVERSAL, playerGuid);

                                if (Player* player = ObjectAccessor::FindPlayer(playerGuid))
                                    player->KilledMonsterCredit(59947);
                            }
                           if (!Is(NPC_JOJO))
                               SetEscortPaused(false);
                            doorEventTimer = 2000;
                            break;
                       case 6:
                           if (Is(NPC_JOJO))
                               SetEscortPaused(false);
                            doorEventTimer = 0;
                            break;
                    }
                }
                else
                    doorEventTimer -= diff;
            }

            npc_escortAI::UpdateAI(diff);
        }
    };
    
    CreatureAI* GetAI(Creature* creature) const
    {
        return new mob_mandori_escortAI(creature);
    }
};

class npc_korga : public CreatureScript
{
    public:
        npc_korga() : CreatureScript("npc_korga") { }

        bool OnQuestAccept(Player* player, Creature* creature, Quest const* quest)
        {
            if (quest->GetQuestId() == 30589) // Détruire l'épave
                if (Creature* jiEscort = player->SummonCreature(60900, 424.71f, 3635.59f, 92.70f, 2.498430f, TEMPSUMMON_MANUAL_DESPAWN, 0, player->GetGUID()))
                    jiEscort->AI()->SetGUID(player->GetGUID());

            return true;
        }
};

class mob_ji_forest_escort : public CreatureScript
{
public:
    mob_ji_forest_escort() : CreatureScript("mob_ji_forest_escort") { }

    struct mob_ji_forest_escortAI : public npc_escortAI
    {        
        mob_ji_forest_escortAI(Creature* creature) : npc_escortAI(creature)
        {}

        uint64 playerGuid;

        uint32 IntroTimer;

        void Reset()
        {
            playerGuid      = 0;

            IntroTimer      = 100;
        }

        void SetGUID(uint64 guid, int32 type)
        {
            playerGuid = guid;
        }
        
        void WaypointReached(uint32 waypointId)
        {}

        void LastWaypointReached()
        {
            if (Player* player = ObjectAccessor::FindPlayer(playerGuid))
                player->AddAura(68483, player); // Phase 16384
        }

        void UpdateAI(const uint32 diff)
        {
            if (IntroTimer)
            {
                if (IntroTimer <= diff)
                {
                    Start(false, true);
                    IntroTimer = 0;
                }
                else
                    IntroTimer -= diff;
            }

            npc_escortAI::UpdateAI(diff);
        }
    };
    
    CreatureAI* GetAI(Creature* creature) const
    {
        return new mob_ji_forest_escortAI(creature);
    }
    
};

class AreaTrigger_at_rescue_soldiers : public AreaTriggerScript
{
    public:
        AreaTrigger_at_rescue_soldiers() : AreaTriggerScript("AreaTrigger_at_rescue_soldiers")
        {}

        bool OnTrigger(Player* player, AreaTriggerEntry const* trigger)
        {
           if (player->GetQuestStatus(29794) != QUEST_STATUS_INCOMPLETE)
               return true;

           if (!player->HasAura(129340))
               return true;

           player->RemoveAurasDueToSpell(129340);
           player->KilledMonsterCredit(55999);

            return true;
        }
};

class npc_hurted_soldier : public CreatureScript
{
public:
    npc_hurted_soldier() : CreatureScript("npc_hurted_soldier") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_hurted_soldierAI (creature);
    }

    struct npc_hurted_soldierAI : public ScriptedAI
    {
        npc_hurted_soldierAI(Creature* creature) : ScriptedAI(creature) {}

        uint32 checkSavedTimer;
        bool HasBeenSaved;

        void Reset()
        {
            checkSavedTimer = 2500;
            HasBeenSaved = false;
            me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
        }

        void OnSpellClick(Unit* Clicker)
        {
            me->RemoveAurasDueToSpell(130966); // Feign Death
            me->EnterVehicle(Clicker);
            me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
            HasBeenSaved = true;
        }

        void UpdateAI(const uint32 diff)
        {
            if (checkSavedTimer)
            {
                if (checkSavedTimer <= diff)
                {
                    if (HasBeenSaved && !me->GetVehicle())
                    {
                        me->MonsterSay("Thanks you, i'll never forget that.", LANG_UNIVERSAL, 0);
                        me->DespawnOrUnsummon(5000);
                        checkSavedTimer = 0;
                    }
                    else
                        checkSavedTimer = 2500;
                }
                else
                    checkSavedTimer -= diff;
            }
        }
    };
};

class boss_vordraka : public CreatureScript
{
public:
    boss_vordraka() : CreatureScript("boss_vordraka") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_vordrakaAI(creature);
    }

    struct boss_vordrakaAI : public ScriptedAI
    {
        boss_vordrakaAI(Creature* creature) : ScriptedAI(creature)
        {}

        EventMap _events;

        enum eEnums
        {
            QUEST_ANCIEN_MAL        = 29798,

            EVENT_DEEP_ATTACK       = 1,
            EVENT_DEEP_SEA_RUPTURE  = 2,

            SPELL_DEEP_ATTACK       = 117287,
            SPELL_DEEP_SEA_RUPTURE  = 117456,
        };

        void Reset()
        {
            _events.ScheduleEvent(EVENT_DEEP_ATTACK, 10000);
            _events.ScheduleEvent(SPELL_DEEP_SEA_RUPTURE, 12500);
        }

        void JustDied(Unit* attacker)
        {
            std::list<Player*> playerList;
            GetPlayerListInGrid(playerList, me, 50.0f);

            for (auto player : playerList)
                if (player->GetQuestStatus(QUEST_ANCIEN_MAL) == QUEST_STATUS_INCOMPLETE)
                    if (player->isAlive())
                        player->KilledMonsterCredit(me->GetEntry());
        }

        void UpdateAI(const uint32 diff)
        {
            _events.Update(diff);

            switch (_events.ExecuteEvent())
            {
                case EVENT_DEEP_ATTACK:
                {
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 25.0f, true))
                        me->CastSpell(target, SPELL_DEEP_ATTACK, false);

                    _events.ScheduleEvent(EVENT_DEEP_ATTACK, 10000);
                    break;
                }
                case EVENT_DEEP_SEA_RUPTURE:
                {
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 25.0f, true))
                        me->CastSpell(target, SPELL_DEEP_SEA_RUPTURE, false);

                    _events.ScheduleEvent(EVENT_DEEP_ATTACK, 10000);
                    break;
                }
            }
        }
    };
};

class mob_aysa_gunship_crash : public CreatureScript
{
    public:
        mob_aysa_gunship_crash() : CreatureScript("mob_aysa_gunship_crash") { }

        bool OnQuestAccept(Player* player, Creature* creature, Quest const* quest)
        {
            if (quest->GetQuestId() == 30767) // Tout risquer
                if (Creature* aysa = player->SummonCreature(60729, creature->GetPositionX(), creature->GetPositionY(), creature->GetPositionZ(), creature->GetOrientation(), TEMPSUMMON_MANUAL_DESPAWN, 0, player->GetGUID()))
                    aysa->AI()->SetGUID(player->GetGUID());

            return true;
        }

        struct mob_aysa_gunship_crashAI : public ScriptedAI
        {        
            mob_aysa_gunship_crashAI(Creature* creature) : ScriptedAI(creature)
            {}

            void DamageTaken(Unit* attacker, uint32& damage)
            {
                if (HealthBelowPct(70))
                    damage = 0;
            }
        };
    
        CreatureAI* GetAI(Creature* creature) const
        {
            return new mob_aysa_gunship_crashAI(creature);
        }
};

class mob_aysa_gunship_crash_escort : public CreatureScript
{
public:
    mob_aysa_gunship_crash_escort() : CreatureScript("mob_aysa_gunship_crash_escort") { }

    struct mob_aysa_gunship_crash_escortAI : public npc_escortAI
    {        
        mob_aysa_gunship_crash_escortAI(Creature* creature) : npc_escortAI(creature)
        {}

        uint64 playerGuid;
        uint64 jiGuid;
        uint64 fireGuid;

        uint32 IntroTimer;
        uint32 discussTimer;

        uint8  discussEvent;

        void Reset()
        {
            playerGuid      = 0;
            jiGuid          = 0;
            fireGuid        = 0;

            IntroTimer      = 100;
            discussTimer    = 0;

            discussEvent    = 0;
        }

        void SetGUID(uint64 guid, int32 type)
        {
            playerGuid = guid;

            if (Creature* ji = me->SummonCreature(60741, 230.31f, 4006.67f, 87.27f, 3.38f, TEMPSUMMON_MANUAL_DESPAWN, 0, guid))
                jiGuid = ji->GetGUID();

            if (GameObject* gob = me->SummonGameObject(215344, 227.75f, 4006.38f, 87.06f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, RESPAWN_IMMEDIATELY, guid))
                fireGuid = gob->GetGUID();
        }

        void WaypointReached(uint32 waypointId)
        {
            if (waypointId == 8)
            {
                SetEscortPaused(true);
                discussTimer = 1000;
            }
        }

        Creature* getJi()
        {
            return me->GetMap()->GetCreature(jiGuid);
        }

        void UpdateAI(const uint32 diff)
        {
            if (IntroTimer)
            {
                if (IntroTimer <= diff)
                {
                    Start(false, true);
                    IntroTimer = 0;
                }
                else
                    IntroTimer -= diff;
            }
            
            if (discussTimer)
            {
                if (discussTimer <= diff)
                {
                    switch (++discussEvent)
                    {
                        case 1:
                            me->MonsterSay("Ji, what are you doing ?! You can't do that !", LANG_UNIVERSAL, playerGuid);
                            if (Creature* ji = getJi())
                                ji->SetFacingToObject(me);
                            discussTimer = 3000;
                            break;
                        case 2:
                            if (Creature* ji = getJi())
                                ji->MonsterSay("We have no choice Aysa.", LANG_UNIVERSAL, playerGuid);
                            discussTimer = 3000;
                            break;
                        case 3:
                            me->MonsterSay("You are going to kill him !", LANG_UNIVERSAL, playerGuid);
                            discussTimer = 3000;
                            break;
                        case 4:
                            if (Creature* ji = getJi())
                                ji->MonsterSay("In our situation, inaction would be the greatest danger.", LANG_UNIVERSAL, playerGuid);
                            discussTimer = 3000;
                            break;
                        case 5:
                            me->MonsterSay("I hope you know what you're doing, Ji...", LANG_UNIVERSAL, playerGuid);
                            discussTimer = 5000;
                            break;
                        case 6:
                            SetEscortPaused(false);
                            
                            if (Creature* ji = getJi())
                                ji->GetMotionMaster()->MovePoint(0, 227.21f, 3981.09f, 85.92f);

                            discussTimer = 1000;
                            break;
                        case 7:
                            if (Player* player = ObjectAccessor::GetPlayer(*me, playerGuid))
                            {
                                player->KilledMonsterCredit(60727);
                                player->SendMovieStart(117);
                            }
                            discussTimer = 500;
                            break;
                        case 8:
                            if (Player* player = ObjectAccessor::GetPlayer(*me, playerGuid))
                                player->NearTeleportTo(249.38f, 3939.55f, 65.61f, 1.501471f);
                            
                            if (Creature* ji = getJi())
                                ji->DespawnOrUnsummon();

                            if (GameObject* gob = me->GetMap()->GetGameObject(fireGuid))
                                gob->Delete();

                            discussTimer = 0;
                            break;
                    }
                }
                else
                    discussTimer -= diff;
            }

            npc_escortAI::UpdateAI(diff);
        }
    };
    
    CreatureAI* GetAI(Creature* creature) const
    {
        return new mob_aysa_gunship_crash_escortAI(creature);
    }
    
};

#define MAX_ENNEMIES_POS   2
#define MAX_HEALER_COUNT   12
#define UPDATE_POWER_TIMER 3000

Position ennemiesPositions[MAX_ENNEMIES_POS] =
{
    {215.0f, 3951.0f, 71.4f},
    {290.0f, 3939.0f, 86.7f}
};

enum eEnums
{
    QUEST_HEALING_SHEN      = 29799,
            
    NPC_HEALER_A            = 60878,
    NPC_HEALER_H            = 60896,
    NPC_ENNEMY              = 60858,

    NPC_SHEN_HEAL_CREDIT    = 56011,

    EVENT_CHECK_PLAYERS     = 1,
    EVENT_UPDATE_POWER      = 2,
    EVENT_SUMMON_ENNEMY     = 3,
    EVENT_SUMMON_HEALER     = 4,

    SPELL_SHEN_HEALING      = 117783,
    SPELL_HEALER_A          = 117784,
    SPELL_HEALER_H          = 117932,
};

class npc_ji_end_event : public CreatureScript
{
public:
    npc_ji_end_event() : CreatureScript("npc_ji_end_event") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_ji_end_eventAI(creature);
    }

    struct npc_ji_end_eventAI : public ScriptedAI
    {
        npc_ji_end_eventAI(Creature* creature) : ScriptedAI(creature), _summons(creature)
        {}

        EventMap   _events;
        SummonList _summons;

        bool       inProgress;
        uint8      healerCount;
        uint8      ennemiesCount;
        uint16     actualPower;

        void Reset()
        {
            _summons.DespawnAll();

            healerCount   = 0;
            ennemiesCount = 0;
            actualPower   = 0;

            inProgress = false;

            _events.Reset();
            _events.ScheduleEvent(EVENT_CHECK_PLAYERS, 5000);
        }

        bool CheckPlayers()
        {
            std::list<Player*> playerList;
            GetPlayerListInGrid(playerList, me, 100.0f);

            for (auto player : playerList)
                if (player->GetQuestStatus(QUEST_HEALING_SHEN) == QUEST_STATUS_INCOMPLETE)
                    if (player->isAlive())
                        return true;

            return false;
        }

        void UpdatePower()
        {
            actualPower = (actualPower + healerCount <= 700) ? actualPower + healerCount: 700;

            std::list<Player*> playerList;
            GetPlayerListInGrid(playerList, me, 100.0f);

            for (auto player : playerList)
            {
                if (player->GetQuestStatus(QUEST_HEALING_SHEN) == QUEST_STATUS_INCOMPLETE)
                {
                    if (player->isAlive())
                    {
                        if (actualPower < 700) // IN_PROGRESS
                        {
                            if (!player->HasAura(SPELL_SHEN_HEALING))
                                player->CastSpell(player, SPELL_SHEN_HEALING, true);

                            player->SetPower(POWER_ALTERNATE_POWER, actualPower);
                        }
                        else
                        {
                            if (player->HasAura(SPELL_SHEN_HEALING))
                                player->RemoveAurasDueToSpell(SPELL_SHEN_HEALING);

                            player->KilledMonsterCredit(NPC_SHEN_HEAL_CREDIT);
                        }
                    }
                }
            }

            if (actualPower >= 700)
                Reset();
        }

        void SummonEnnemy()
        {
            uint8 pos = rand() % MAX_ENNEMIES_POS;
            float posJumpX = frand(228.0f, 270.0f);
            float posJumpY = frand(3949.0f, 3962.0f);

            if (Creature* ennemy = me->SummonCreature(NPC_ENNEMY, ennemiesPositions[pos].GetPositionX(), ennemiesPositions[pos].GetPositionY(), ennemiesPositions[pos].GetPositionZ(), TEMPSUMMON_CORPSE_DESPAWN))
                ennemy->GetMotionMaster()->MoveJump(posJumpX, posJumpY, me->GetMap()->GetHeight(me->GetPhaseMask(), posJumpX, posJumpY, 100.0f), 20.0f, 20.0f);
        }

        void SummonHealer()
        {
            uint32 entry = rand() % 2 ? NPC_HEALER_A: NPC_HEALER_H;
            float posX = frand(228.0f, 270.0f);
            float posY = frand(3949.0f, 3962.0f);

            me->SummonCreature(entry, posX, posY, me->GetMap()->GetHeight(me->GetPhaseMask(), posX, posY, 100.0f), 1.37f, TEMPSUMMON_CORPSE_DESPAWN);
        }

        void JustSummoned(Creature* summon)
        {
            _summons.Summon(summon);

            switch (summon->GetEntry())
            {
                case NPC_HEALER_A:
                case NPC_HEALER_H:
                    ++healerCount;
                    break;
                case NPC_ENNEMY:
                    ++ennemiesCount;
                    break;
            }
        }

        void SummonedCreatureDies(Creature* summon, Unit* /*killer*/)
        {
            _summons.Despawn(summon);

            switch (summon->GetEntry())
            {
                case NPC_HEALER_A:
                case NPC_HEALER_H:
                    --healerCount;
                    break;
                case NPC_ENNEMY:
                    --ennemiesCount;
                    break;
            }
        }

        void UpdateAI(const uint32 diff)
        {
            _events.Update(diff);

            switch (_events.ExecuteEvent())
            {
                case EVENT_CHECK_PLAYERS:
                {
                    bool playerNearWithQuest = CheckPlayers();

                    if (inProgress && !playerNearWithQuest)
                    {
                        inProgress = false;
                        Reset();
                    }
                    else if (!inProgress && playerNearWithQuest)
                    {
                        inProgress = true;
                        _events.ScheduleEvent(EVENT_UPDATE_POWER,  UPDATE_POWER_TIMER);
                        _events.ScheduleEvent(EVENT_SUMMON_ENNEMY, 5000);
                        _events.ScheduleEvent(EVENT_SUMMON_HEALER, 5000);
                    }
                    _events.ScheduleEvent(EVENT_CHECK_PLAYERS, 5000);
                    break;
                }
                case EVENT_UPDATE_POWER:
                    UpdatePower();
                    _events.ScheduleEvent(EVENT_UPDATE_POWER, UPDATE_POWER_TIMER);
                    break;
                case EVENT_SUMMON_ENNEMY:
                    if (ennemiesCount < (healerCount / 2))
                    {
                        SummonEnnemy();
                        _events.ScheduleEvent(EVENT_SUMMON_ENNEMY, 5000);
                    }
                    else
                        _events.ScheduleEvent(EVENT_SUMMON_ENNEMY, 7500);
                    break;
                case EVENT_SUMMON_HEALER:
                    if (healerCount < MAX_HEALER_COUNT)
                        SummonHealer();

                    _events.ScheduleEvent(EVENT_SUMMON_HEALER, 12500);
                    break;
            }
        }
    };
};

class npc_shen_healer : public CreatureScript
{
    public:
        npc_shen_healer() : CreatureScript("npc_shen_healer") { }

        struct npc_shen_healerAI : public ScriptedAI
        {        
            npc_shen_healerAI(Creature* creature) : ScriptedAI(creature)
            {}

            void Reset()
            {
                me->SetReactState(REACT_PASSIVE);
                me->CastSpell(me, me->GetEntry() == NPC_HEALER_A ? SPELL_HEALER_A: SPELL_HEALER_H, true);
            }

            void EnterCombat(Unit*)
            {
                return;
            }
        };
    
        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_shen_healerAI(creature);
        }
};

#define GOSSIP_CHOOSE_FACTION     "I'm ready to choose my destiny."
#define GOSSIP_TP_STORMIND        "I would like to go to Stormwind"
#define GOSSIP_TP_ORGRI           "I would like to go to Orgrimmar"

class npc_shang_xi_choose_faction : public CreatureScript
{
    public:
        npc_shang_xi_choose_faction() : CreatureScript("npc_shang_xi_choose_faction") { }

        bool OnGossipHello(Player* player, Creature* creature)
        {
            if (creature->isQuestGiver())
                player->PrepareQuestMenu(creature->GetGUID());

            if (player->getRace() == RACE_PANDAREN_NEUTRAL)
            {
                if (player->GetQuestStatus(31450) == QUEST_STATUS_INCOMPLETE)
                    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_CHOOSE_FACTION, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
            }
            else if (player->getRace() == RACE_PANDAREN_ALLI)
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_TP_STORMIND, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
            else if (player->getRace() == RACE_PANDAREN_HORDE)
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_TP_ORGRI, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);

            player->PlayerTalkClass->SendGossipMenu(1, creature->GetGUID());
            return true;
        }

        bool OnGossipSelect(Player* player, Creature* /*creature*/, uint32 /*sender*/, uint32 action)
        {
            if (action == GOSSIP_ACTION_INFO_DEF + 1)
                player->ShowNeutralPlayerFactionSelectUI();
            else if (action == GOSSIP_ACTION_INFO_DEF + 2)
                player->TeleportTo(0, -8866.55f, 671.93f, 97.90f, 5.31f);
            else if (action == GOSSIP_ACTION_INFO_DEF + 3)
                player->TeleportTo(1, 1577.30f, -4453.64f, 15.68f, 1.84f);

            player->PlayerTalkClass->SendCloseGossip();
            return true;
        }
};
