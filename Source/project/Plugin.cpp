#include "stdafx.h"
#include "Plugin.h"
#include "IExamInterface.h"
#include "EliteAI/EliteBehaviorTree/EBehaviorTree.h"
#include "EliteAI/EliteBehaviorTree/EBlackboard.h"
#include "Behaviours.h"

//Called only once, during initialization
void Plugin::Initialize(IBaseInterface* pInterface, PluginInfo& info)
{
	//Retrieving the interface
	//This interface gives you access to certain actions the AI_Framework can perform for you
	m_pInterface = static_cast<IExamInterface*>(pInterface);
	//Bit information about the plugin
	//Please fill this in!!
	info.BotName = "Storm Trooper V2";
	info.Student_FirstName = "Jens";
	info.Student_LastName = "Eeckhout";
	info.Student_Class = "2DAE07";

	m_VecHouses = vector<extendedHouseInfo>();
	m_VecInventory = vector<InventorySlotInfo>();

	//Default initialize ItemVector
	for (int i{}; i < 5; ++i)
	{
		m_VecInventory.push_back(InventorySlotInfo{});
	}

	//Set BehaviourTree
	auto pBlackboard = new Elite::Blackboard;
	pBlackboard->AddData("Health", 0.f);
	pBlackboard->AddData("AngularVelocity", m_AngularVelocity);
	pBlackboard->AddData("RecentlyVisitedHouse", RecentlyVisitedHouse);
	pBlackboard->AddData("CurrEscapeTimer", m_CurrEscapeTimer);
	pBlackboard->AddData("CurrWanderTimer", m_CurrWanderTimer);
	pBlackboard->AddData("MaxWanderTimer", m_DefaultWanderInHouseTimer);
	pBlackboard->AddData("MaxEscapeTimer", m_DefaultEscapeTimer);
	pBlackboard->AddData("IsEscaping", m_IsEscaping);
	pBlackboard->AddData("IsWandering", m_IsWanderingInHouse);
	pBlackboard->AddData("Energy", 0.f);
	pBlackboard->AddData("SteeringBehaviour", m_currSteeringB);
	pBlackboard->AddData("Agent", m_Agent);
	pBlackboard->AddData("HouseMap", &m_VecHouses);
	pBlackboard->AddData("Inventory", &m_VecInventory);
	pBlackboard->AddData("ItemMap", &m_VecItems);
	pBlackboard->AddData("Target", m_Target);
	pBlackboard->AddData("Checkpoint", m_CheckpointLocation);
	pBlackboard->AddData("ReachedCheckpoint", m_HasReachedCheckpoint);
	pBlackboard->AddData("Interface", m_pInterface);
	pBlackboard->AddData("Enemy", EntityInfo());
	pBlackboard->AddData("EnemiesInFOV", false);
	pBlackboard->AddData("FoundNewHouse", false);
	pBlackboard->AddData("NewHouse", HouseInfo());
	pBlackboard->AddData("currSeekTimer", m_CurrSeekTimer);
	pBlackboard->AddData("MaxEvadeEnemyTimer", m_DefaultFleeTimer);
	pBlackboard->AddData("CurrEvadeTimer", m_CurrFleeTimer);
	pBlackboard->AddData("AngleChange", m_AngleChange);
	pBlackboard->AddData("WanderAngle", m_WanderAngle);
	pBlackboard->AddData("WanderRadius", m_Radius);
	pBlackboard->AddData("CanRun", m_CanRun);

	m_pBehaviorTree = new Elite::BehaviorTree{ pBlackboard,



		new Elite::BehaviorSelector
		({

			new Elite::BehaviorAction(SetCheckpointInWorld),

			new Elite::BehaviorSequence
			({
				new Elite::BehaviorInverter(HasSufficientEnergy),
				new Elite::BehaviorAction(TryFood),
			}),

			new Elite::BehaviorSequence
			({
				new Elite::BehaviorInverter(HasSufficientHealth),
				new Elite::BehaviorAction(TryMedkit),
			}),

			new Elite::BehaviorAction(EscapeFromEnemy),

			new Elite::BehaviorSequence
			({
				new Elite::BehaviorConditional(EnemiesInFOV),
				new Elite::BehaviorSelector
				({
					new Elite::BehaviorSequence
					({
						new Elite::BehaviorConditional(HasGun),
						new Elite::BehaviorSelector
						({
							new Elite::BehaviorSequence
							({
								new Elite::BehaviorConditional(IsFacingEnemy),
								new Elite::BehaviorAction(TryGun)
							}),
							new Elite::BehaviorAction(FaceToEnemy)
						}),
					}),
					new Elite::BehaviorAction(EvadeEnemy)
				}),
			}),

			new Elite::BehaviorSequence
			({
				new Elite::BehaviorConditional(NewHouseWasFound),
				new Elite::BehaviorAction(InspectNewHouse),
			}),

			new Elite::BehaviorSequence
				({
					new Elite::BehaviorConditional(HasItemInMap),
					new Elite::BehaviorInverter(IsInventoryFull),
					new Elite::BehaviorAction(SeekToItem),
				}),

			new Elite::BehaviorAction(EscapeHouse),


			new Elite::BehaviorSequence
			({
				new Elite::BehaviorConditional(HasSufficientHealth),
				new Elite::BehaviorConditional(HasSufficientEnergy),
				new Elite::BehaviorConditional(IsInventoryFull),
				new Elite::BehaviorAction(SeekToTarget),
			}),

			new Elite::BehaviorSelector
			({

				new Elite::BehaviorSequence
				({
					new Elite::BehaviorConditional(HasSufficientHealth),
					new Elite::BehaviorConditional(HasSufficientEnergy),
					new Elite::BehaviorConditional(HasItemInMap),
					new Elite::BehaviorAction(SeekToItem)
				}),

				new Elite::BehaviorSequence
				({
					new Elite::BehaviorConditional(IsInventoryFull),
					new Elite::BehaviorAction(SeekToTarget),
				}),
				
				new Elite::BehaviorAction(SeekToTarget),
			})
		})
	};
}

//Called only once
void Plugin::DllInit()
{
	//Called when the plugin is loaded

}

//Called only once
void Plugin::DllShutdown()
{
	//Called wheb the plugin gets unloaded
	delete m_pBehaviorTree;
	m_pBehaviorTree = nullptr;
}

//Called only once, during initialization
void Plugin::InitGameDebugParams(GameDebugParams& params)
{
	params.AutoFollowCam = true; //Automatically follow the AI? (Default = true)
	params.RenderUI = true; //Render the IMGUI Panel? (Default = true)
	params.SpawnEnemies = true; //Do you want to spawn enemies? (Default = true)
	params.EnemyCount = 20; //How many enemies? (Default = 20)
	params.GodMode = false; //GodMode > You can't die, can be usefull to inspect certain behaviours (Default = false)
							//params.LevelFile = "LevelTwo.gppl";
	params.AutoGrabClosestItem = false; //A call to Item_Grab(...) returns the closest item that can be grabbed. (EntityInfo argument is ignored)
}

//Only Active in DEBUG Mode
//(=Use only for Debug Purposes)
void Plugin::Update(float dt)
{
	//Demo Event Code
	//In the end your AI should be able to walk around without external input
	if (m_pInterface->Input_IsMouseButtonUp(Elite::InputMouseButton::eLeft))
	{
		//Update target based on input
		Elite::MouseData mouseData = m_pInterface->Input_GetMouseData(Elite::InputType::eMouseButton, Elite::InputMouseButton::eLeft);
		const Elite::Vector2 pos = Elite::Vector2(static_cast<float>(mouseData.X), static_cast<float>(mouseData.Y));
		m_Target = m_pInterface->Debug_ConvertScreenToWorld(pos);
	}
	else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_Space))
	{
		m_CanRun = true;
	}
	else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_Left))
	{
		m_AngSpeed -= Elite::ToRadians(10);
	}
	else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_Right))
	{
		m_AngSpeed += Elite::ToRadians(10);
	}
	else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_G))
	{
		m_GrabItem = true;
	}
	else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_U))
	{
		m_UseItem = true;
	}
	else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_R))
	{
		m_RemoveItem = true;
	}
	else if (m_pInterface->Input_IsKeyboardKeyUp(Elite::eScancode_Space))
	{
		m_CanRun = false;
	}
}

//Update
//This function calculates the new SteeringOutput, called once per frame
SteeringPlugin_Output Plugin::UpdateSteering(float dt)
{
	auto steering = SteeringPlugin_Output();
	auto agentInfo = m_pInterface->Agent_GetInfo();
	m_pBehaviorTree->GetBlackboard()->ChangeData("Health", agentInfo.Health);
	m_pBehaviorTree->GetBlackboard()->ChangeData("Energy", agentInfo.Energy);
	m_pBehaviorTree->GetBlackboard()->ChangeData("Enemy", m_CurrEnemy);
	m_pBehaviorTree->GetBlackboard()->GetData("CurrEscapeTimer", m_CurrEscapeTimer);
	m_pBehaviorTree->GetBlackboard()->GetData("Checkpoint", m_CheckpointLocation);
	m_pBehaviorTree->Update();
	m_pBehaviorTree->GetBlackboard()->GetData("AngularVelocity", m_AngularVelocity);
	if (m_IsWanderingInHouse)
	{
		m_CurrWanderTimer += dt;
		if (m_CurrWanderTimer >= m_DefaultWanderInHouseTimer)
		{
			m_IsWanderingInHouse = false;
			m_pBehaviorTree->GetBlackboard()->ChangeData("IsWandering", m_IsWanderingInHouse);
			m_CurrWanderTimer = 0;
			m_CurrEscapeTimer = 0;
			m_pBehaviorTree->GetBlackboard()->ChangeData("CurrEscapeTimer", m_CurrEscapeTimer);
		}
	}
	if (m_IsEscaping && !m_IsWanderingInHouse)
	{
		m_CurrEscapeTimer += dt;
		m_pBehaviorTree->GetBlackboard()->ChangeData("CurrEscapeTimer", m_CurrEscapeTimer);
	}
	m_pBehaviorTree->GetBlackboard()->GetData("Target", m_Target);
	m_pBehaviorTree->GetBlackboard()->GetData("IsEscaping", m_IsEscaping);

	if (agentInfo.Stamina < 1.f)
	{
		m_CanRun = false;
		m_pBehaviorTree->GetBlackboard()->ChangeData("CanRun", m_CanRun);
	}


	//Use the Interface (IAssignmentInterface) to 'interface' with the AI_Framework
	

	CurrentSteeringBehaviour behaviour;
	m_pBehaviorTree->GetBlackboard()->GetData("SteeringBehaviour", behaviour);

	//Use the navmesh to calculate the next navmesh point
	if (behaviour != CurrentSteeringBehaviour::FACE)
	{
		m_Target = m_pInterface->NavMesh_GetClosestPathPoint(m_Target);
	}

	

	//OR, Use the mouse target
	auto nextTargetPos = m_Target; //Uncomment this to use mouse position as guidance

	auto vHousesInFOV = GetHousesInFOV();//uses m_pInterface->Fov_GetHouseByIndex(...)
	auto vEntitiesInFOV = GetEntitiesInFOV(); //uses m_pInterface->Fov_GetEntityByIndex(...)

	Elite::Vector2 entityLocation{};
	UpdateHouses(vHousesInFOV, agentInfo, dt);


	m_pBehaviorTree->GetBlackboard()->ChangeData("currSeekTimer", m_CurrSeekTimer);

	m_pBehaviorTree->GetBlackboard()->ChangeData("EnemiesInFOV", false);

	HandleEntities(vEntitiesInFOV, agentInfo);

	//INVENTORY USAGE DEMO
	//********************


	UpdateItems(agentInfo, vEntitiesInFOV);

	if (m_UseItem)
	{
		//Use an item (make sure there is an item at the given inventory slot)
		m_pInterface->Inventory_UseItem(0);
	}

	if (m_RemoveItem)
	{
		//Remove an item from a inventory slot
		m_pInterface->Inventory_RemoveItem(0);
	}

	

	//Wander
	if (behaviour == CurrentSteeringBehaviour::WANDER || behaviour == CurrentSteeringBehaviour::FLEE)
	{
		auto offset = agentInfo.LinearVelocity;
		offset.Normalize();
		offset *= m_Offset;


		//WanderCircle Offset (Polar to Cartesian Coordinates)
		Elite::Vector2 circleOffset = { cos(m_WanderAngle) * m_Radius, sin(m_WanderAngle) * m_Radius };

		//Change the WanderAngle slightly for next frame
		m_WanderAngle += Elite::randomFloat() * m_AngleChange - (m_AngleChange * .5f); //RAND[-angleChange/2,angleChange/2]

		m_Target = agentInfo.Position + offset + circleOffset;
	}

	//Simple Seek Behaviour (towards Target)
	if (behaviour != CurrentSteeringBehaviour::FACE)
	{
		steering.AutoOrientate = true;
		steering.LinearVelocity = m_Target - agentInfo.Position; //Desired Velocity
		steering.LinearVelocity.Normalize(); //Normalize Desired Velocity
		steering.LinearVelocity *= agentInfo.MaxLinearSpeed; //Rescale to Max Speed
	}
	else if(behaviour == CurrentSteeringBehaviour::FACE)
	{
		steering.AutoOrientate = false;
		auto agentRot = agentInfo.Orientation;
		agentRot -= float(M_PI) / 2;
		float enemyRot = Elite::GetOrientationFromVelocity(m_Target);
		steering.AngularVelocity = agentRot - enemyRot;
		if (steering.AngularVelocity > 0)
			steering.AngularVelocity *= 1;
		else
			steering.AngularVelocity *= -1;
		steering.AngularVelocity *= agentInfo.MaxAngularSpeed;

		steering.LinearVelocity = m_Target - agentInfo.Position; //Desired Velocity
		steering.LinearVelocity.Normalize(); //Normalize Desired Velocity
		steering.LinearVelocity *= agentInfo.MaxLinearSpeed; //Rescale to Max Speed
	}

	//Flee
	//if (behaviour == CurrentSteeringBehaviour::FLEE)
	//{
	//	for (auto& e : vEntitiesInFOV)
	//	{
	//		if (e.Type == eEntityType::ENEMY)
	//		{
	//			EnemyInfo enemyInfo;
	//			m_pInterface->Enemy_GetInfo(e, enemyInfo);
	//			auto ahead = agentInfo.Position + Elite::GetNormalized(steering.LinearVelocity) * m_MaxSeeAhead;
	//			auto distance = Elite::Distance(enemyInfo.Location, ahead);
	//			if (distance <= enemyInfo.Size * 30)
	//			{
	//				auto avoidance_force = ahead - enemyInfo.Location;
	//				avoidance_force = Elite::GetNormalized(avoidance_force) * 20;
	//				Elite::Vector2 avoidance{};
	//				avoidance.x = ahead.x - enemyInfo.Location.x;
	//				avoidance.y = ahead.y - enemyInfo.Location.y;
	//				avoidance = Elite::GetNormalized(avoidance);
	//				avoidance *= 20;
	//				steering.LinearVelocity += avoidance;
	//			}
	//		}
	//	}
	//}

	for (auto& e : vEntitiesInFOV)
	{
		if (e.Type == eEntityType::PURGEZONE)
		{
			PurgeZoneInfo zoneInfo;
			m_pInterface->PurgeZone_GetInfo(e, zoneInfo);
			auto ahead = agentInfo.Position + Elite::GetNormalized(steering.LinearVelocity) * m_MaxSeeAhead;
			auto distance = Elite::Distance(zoneInfo.Center, ahead);
			if (distance <= zoneInfo.Radius)
			{
				auto avoidance_force = ahead - zoneInfo.Center;
				avoidance_force = Elite::GetNormalized(avoidance_force) * 10;
				Elite::Vector2 avoidance{};
				avoidance.x = ahead.x - zoneInfo.Center.x;
				avoidance.y = ahead.y - zoneInfo.Center.y;
				avoidance = Elite::GetNormalized(avoidance);
				avoidance *= 10;
				steering.LinearVelocity += avoidance;
			}
		}
	}

	m_pBehaviorTree->GetBlackboard()->GetData("CanRun", m_CanRun);
	if (agentInfo.Stamina <= 0.f)
	{
		m_pBehaviorTree->GetBlackboard()->ChangeData("CanRun", false);
	}
	steering.RunMode = m_CanRun; //If RunMode is True > MaxLinSpd is increased for a limited time (till your stamina runs out)

	//@End (Demo Purposes)
	 //Reset State
	m_UseItem = false;
	m_RemoveItem = false;

	previousLocationEnemy = m_EvadingEntity.Location;

	for (auto& e : vEntitiesInFOV)
	{
		if (e.Type == eEntityType::PURGEZONE)
		{
			PurgeZoneInfo zoneInfo;
			m_pInterface->PurgeZone_GetInfo(e, zoneInfo);
			std::cout << "Purge Zone in FOV:" << e.Location.x << ", " << e.Location.y << " ---EntityHash: " << e.EntityHash << "---Radius: " << zoneInfo.Radius << std::endl;
		}
	}

	m_pBehaviorTree->GetBlackboard()->ChangeData("Target", m_Target);
	m_pBehaviorTree->GetBlackboard()->ChangeData("Agent", agentInfo);

	auto agentRot = agentInfo.Orientation;
	agentRot -= float(M_PI) / 2;
	float cosRot = std::cos(agentRot);
	float sinRot = std::sin(agentRot);

	float ValueX = agentInfo.FOV_Range * cosRot;
	float ValueY = agentInfo.FOV_Range * sinRot;

	Elite::Vector2 LookAt(ValueX, ValueY);
	Elite::Vector2 debugPoint(agentInfo.Position + LookAt);
	m_pInterface->Draw_Point(debugPoint, 10, Elite::Vector3(0, 0, 1));

	m_pBehaviorTree->GetBlackboard()->GetData("ReachedCheckpoint", m_HasReachedCheckpoint);
	Elite::Vector2 toTarget{m_CheckpointLocation - agentInfo.Position };
	float distance = toTarget.Magnitude();
	if (distance < 3.5f)
	{
		m_HasReachedCheckpoint = true;
		m_pBehaviorTree->GetBlackboard()->ChangeData("ReachedCheckpoint", m_HasReachedCheckpoint);
	}

	return steering;
}

void Plugin::UpdateHouses(vector<HouseInfo>& vHouses, AgentInfo& agent, float& dt)
{
	bool foundNewHouse{};
	m_pBehaviorTree->GetBlackboard()->GetData("FoundNewHouse", foundNewHouse);
	for (int i = 0; i < int(vHouses.size()); ++i)
	{
		bool foundInMap{ false };
		if (!m_FirstHouse)
		{
			extendedHouseInfo newHouse = extendedHouseInfo{ vHouses[i],false };
			m_VecHouses.push_back(newHouse);

				m_pBehaviorTree->GetBlackboard()->ChangeData("FoundNewHouse", true);
				m_pBehaviorTree->GetBlackboard()->ChangeData("NewHouse", newHouse.info);
				m_MostRecentHouse = newHouse.info;
			m_FirstHouse = true;
		}

		for (extendedHouseInfo value : m_VecHouses)
		{
			if (value.info.Center == vHouses[i].Center)
			{
				foundInMap = true;
			}
		}

		if (!foundInMap && !foundNewHouse)
		{
			extendedHouseInfo newHouse = extendedHouseInfo{ vHouses[i],false };
			m_VecHouses.push_back(newHouse);

				m_pBehaviorTree->GetBlackboard()->ChangeData("FoundNewHouse", true);
				m_pBehaviorTree->GetBlackboard()->ChangeData("NewHouse", newHouse.info);
				m_MostRecentHouse = newHouse.info;
		}
	}

	for (extendedHouseInfo& value : m_VecHouses)
	{
		Elite::Vector2 toTarget{ value.info.Center - agent.Position };
		float distance = toTarget.Magnitude();
		if (distance < 2.0f)
		{
			value.RecentlyVisited = true;
			if (value.info.Center == m_MostRecentHouse.Center)
			{
				m_pBehaviorTree->GetBlackboard()->ChangeData("FoundNewHouse", false);
				m_pBehaviorTree->GetBlackboard()->ChangeData("RecentlyVisitedHouse", true);
				m_IsWanderingInHouse = true;
				m_pBehaviorTree->GetBlackboard()->ChangeData("IsWandering", m_IsWanderingInHouse);
			}
		}

		if (value.RecentlyVisited)
		{
			value.TimeSinceLastVisit += dt;
		}

		if (value.TimeSinceLastVisit >= m_ResetVisitedTimer)
		{
			std::cout << "A house can be visited again" << std::endl;
			value.RecentlyVisited = false;
			value.TimeSinceLastVisit = 0;
		}
	}
}

void Plugin::UpdateItems(AgentInfo& agent, vector<EntityInfo>& vEntitiesInFOV)
{
	if (m_GrabItem)
	{
		ItemInfo item_Pickup;
		//Item_Grab > When DebugParams.AutoGrabClosestItem is TRUE, the Item_Grab function returns the closest item in range
		//Keep in mind that DebugParams are only used for debugging purposes, by default this flag is FALSE
		//Otherwise, use GetEntitiesInFOV() to retrieve a vector of all entities in the FOV (EntityInfo)
		//Item_Grab gives you the ItemInfo back, based on the passed EntityHash (retrieved by GetEntitiesInFOV)
		for (EntityInfo entity : vEntitiesInFOV)
		{
			if (entity.Type == eEntityType::ITEM)
			{
				if (abs(Elite::Distance(entity.Location, agent.Position)) <= agent.GrabRange)
				{
					if (m_pInterface->Item_Grab(entity, item_Pickup))
					{
						//Once grabbed, you can add it to a specific inventory slot
						//Slot must be empty
						bool itemAdded{ false };
						int nrOfPistols{};
						int nrOfFood{};

						for (int i{}; i < int(m_VecItems.size()); ++i)
						{
							if (m_VecItems[i].Location == entity.Location)
							{
								std::swap(m_VecItems[i], m_VecItems[m_VecItems.size() - 1]);
								m_VecItems.pop_back();
							}
						}

						for (int count{}; count < 5; ++count)
						{
							ItemInfo item;
							if (m_VecInventory[count].InUse)
							{
								m_pInterface->Inventory_GetItem(count, item);
								switch (item.Type)
								{
								case eItemType::PISTOL:
									++nrOfPistols;
									break;

								case eItemType::MEDKIT:
									break;

								case eItemType::FOOD:
									++nrOfFood;
									break;

								default:
									break;
								}
							}

						}
						for (int i = 0; i < int(m_pInterface->Inventory_GetCapacity()); ++i)
						{
							if (!itemAdded)
							{
								if (!m_VecInventory[i].InUse)
								{
									m_pInterface->Inventory_AddItem(UINT(i), item_Pickup);
									m_GrabItem = false;
									m_VecInventory[i].InUse = true;
									itemAdded = true;
									EntityInfo RemoveAtValue{};
									bool FoundItemToRemove{};
									if (item_Pickup.Type == eItemType::GARBAGE)
									{
										m_pInterface->Inventory_RemoveItem(UINT(i));
										m_VecInventory[i].InUse = false;
									}

									if (item_Pickup.Type == eItemType::PISTOL && nrOfPistols >= 3)
									{
										m_pInterface->Inventory_RemoveItem(UINT(i));
										m_VecInventory[i].InUse = false;
									}

									if (item_Pickup.Type == eItemType::FOOD && nrOfFood >= 2)
									{
										m_pInterface->Inventory_UseItem(i);
										m_pInterface->Inventory_RemoveItem(UINT(i));
										m_VecInventory[i].InUse = false;
									}

								}

							}
						}
					}
				}
			}
		}







	}
}

void Plugin::HandleEntities(vector<EntityInfo>& vEntitiesInFOV, AgentInfo& agent)
{
	bool usedItem{ false };
	bool hasFoundEnemy{false};
	bool shouldFlee{ false };

	for (EntityInfo entity : vEntitiesInFOV)
	{

		if (entity.Type == eEntityType::ENEMY && !hasFoundEnemy)
		{
			hasFoundEnemy = true;
			m_CurrEnemy = entity;
			m_pBehaviorTree->GetBlackboard()->ChangeData("EnemiesInFOV", true);
			m_pBehaviorTree->GetBlackboard()->ChangeData("Enemy", m_CurrEnemy);

			shouldFlee = true;
			for (int i = 0; i < int(m_pInterface->Inventory_GetCapacity()); ++i)
			{
				if (m_VecInventory[i].InUse && !usedItem)
				{
					ItemInfo info{};
					m_pInterface->Inventory_GetItem(i, info);
					if (info.Type == eItemType::PISTOL)
					{
						auto linVelocityEnemy = previousLocationEnemy - entity.Location;
						Elite::Normalize(linVelocityEnemy);
						shouldFlee = false;
						m_EvadingEntity = entity;
						m_Target = entity.Location + linVelocityEnemy;
					}
				}

			}
			if (shouldFlee)
			{
				m_EvadingEntity = entity;
				m_CanRun = true;
				m_pBehaviorTree->GetBlackboard()->ChangeData("CanRun", m_CanRun);
				m_CurrFleeTimer = m_DefaultFleeTimer;
			}

		}

		if (entity.Type == eEntityType::ITEM)
		{
			if (!IsInventoryFull(m_pBehaviorTree->GetBlackboard()))
			{
				m_Target = entity.Location;
				m_IsSeekingToItem = true;
				if (abs(m_Target.x - agent.Position.x) < 1.5f && abs(m_Target.y - agent.Position.y) < 1.5f)
				{
					m_GrabItem = true;
				}
			}

			bool foundInMap{ false };


			for (EntityInfo value : m_VecItems)
			{
				if (value.Location == entity.Location)
				{
					foundInMap = true;
				}
			}

			if (!foundInMap)
			{
				m_VecItems.push_back(entity);

			}
		}
	}
}

//This function should only be used for rendering debug elements
void Plugin::Render(float dt) const
{
	//This Render function should only contain calls to Interface->Draw_... functions
	m_pInterface->Draw_SolidCircle(m_Target, .7f, { 0,0 }, { 1, 0, 0 });
}

vector<HouseInfo> Plugin::GetHousesInFOV() const
{
	vector<HouseInfo> vHousesInFOV = {};

	HouseInfo hi = {};
	for (int i = 0;; ++i)
	{
		if (m_pInterface->Fov_GetHouseByIndex(i, hi))
		{
			vHousesInFOV.push_back(hi);
			continue;
		}

		break;
	}

	return vHousesInFOV;
}

vector<EntityInfo> Plugin::GetEntitiesInFOV() const
{
	vector<EntityInfo> vEntitiesInFOV = {};

	EntityInfo ei = {};
	for (int i = 0;; ++i)
	{
		if (m_pInterface->Fov_GetEntityByIndex(i, ei))
		{
			vEntitiesInFOV.push_back(ei);
			continue;
		}

		break;
	}

	return vEntitiesInFOV;
}
