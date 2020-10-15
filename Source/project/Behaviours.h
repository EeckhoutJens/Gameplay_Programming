#pragma once
#include "EliteAI/EliteBehaviorTree/EBehaviorTree.h"
#include "EliteMath/EMath.h"
#include <random>

inline bool HasSufficientHealth(Elite::Blackboard* pBlackboard)
{
	float Health{};
	bool DataAvailable = pBlackboard->GetData("Health",Health);
	if (!DataAvailable)
	{
		return false;
	}
	if (Health < 5.0f)
	{
		return false;
	}

	
	return true;
};

inline Elite::BehaviorState EscapeFromEnemy(Elite::Blackboard* pBlackboard)
{
	AgentInfo agent;
	bool canRun;

	bool dataAvailable = pBlackboard->GetData("Agent", agent) && pBlackboard->GetData("CanRun",canRun);

	if (!agent.WasBitten || !dataAvailable)
	{
		return Elite::Failure;
	}
	
	canRun = true;
	pBlackboard->ChangeData("CanRun", canRun);
	return Elite::Success;

}

inline Elite::BehaviorState SeekToHouse(Elite::Blackboard* pBlackboard)
{
	IExamInterface* m_pInterface{};
	std::vector<extendedHouseInfo>* m_pHouseMap{};
	CurrentSteeringBehaviour sB;
	Elite::Vector2 m_Target{};
	const bool DataAvailable = pBlackboard->GetData("SteeringBehaviour", sB) && pBlackboard->GetData("Interface", m_pInterface) && pBlackboard->GetData("HouseMap", m_pHouseMap) && pBlackboard->GetData("Target", m_Target);
	if (!DataAvailable)
	{
		return Elite::Failure;
	}

	Elite::Vector2 newTarget;
	bool FirstCheck{};
	auto agentInfo = m_pInterface->Agent_GetInfo();

	for (extendedHouseInfo value : *m_pHouseMap)
	{
		if (!FirstCheck && !value.RecentlyVisited)
		{
			newTarget = value.info.Center;
			m_Target = m_pInterface->NavMesh_GetClosestPathPoint(newTarget);
		}


		else if (abs(Elite::Distance(value.info.Center, agentInfo.Position)) < abs(Elite::Distance(newTarget, agentInfo.Position)) && !value.RecentlyVisited)
		{
			newTarget = value.info.Center;
			m_Target = m_pInterface->NavMesh_GetClosestPathPoint(newTarget);
		}
	}

	sB = CurrentSteeringBehaviour::SEEK;
	pBlackboard->ChangeData("SteeringBehaviour", sB);
	pBlackboard->ChangeData("Target", m_Target);
	pBlackboard->ChangeData("Checkpoint", m_Target);
	return  Elite::Success;



}

inline Elite::BehaviorState SetCheckpointInWorld(Elite::Blackboard* pBlackboard)
{
	IExamInterface* pInterface = nullptr;
	bool hasReachedCheckpoint;
	std::vector<extendedHouseInfo>* m_pHouseMap{};

	bool dataAvailable = pBlackboard->GetData("Interface", pInterface) && pBlackboard->GetData("ReachedCheckpoint",hasReachedCheckpoint) && pBlackboard->GetData("HouseMap", m_pHouseMap);
	if (!dataAvailable || !hasReachedCheckpoint)
	{
		return Elite::Failure;
	}
	WorldInfo info = pInterface->World_GetInfo();
	Elite::Vector2 newTarget{};
	Elite::Vector2 targetToAssign{};
	auto agentInfo = pInterface->Agent_GetInfo();
	bool FirstCheck{ false };

	//Set up random generator
	if (m_pHouseMap->size() >= 5)
	{
		for (extendedHouseInfo value : *m_pHouseMap)
		{
			if (!FirstCheck && !value.RecentlyVisited)
			{
				FirstCheck = true;
				newTarget = value.info.Center;
				targetToAssign = pInterface->NavMesh_GetClosestPathPoint(newTarget);
			}


			else if (abs(Elite::Distance(value.info.Center, agentInfo.Position)) < abs(Elite::Distance(newTarget, agentInfo.Position)) && !value.RecentlyVisited)
			{
				newTarget = value.info.Center;
				targetToAssign = pInterface->NavMesh_GetClosestPathPoint(newTarget);
			}
		}
		pBlackboard->ChangeData("Checkpoint", targetToAssign);
		pBlackboard->ChangeData("ReachedCheckpoint", false);
		if (!FirstCheck)
		{
			return Elite::Success;
		}
	}
	std::random_device randomDevice;
	std::mt19937 generator(randomDevice());
	std::uniform_int_distribution<> xDistr(-(info.Dimensions.x / 2), info.Dimensions.x / 2);
	std::uniform_int_distribution<> yDistr(-(info.Dimensions.y / 2), info.Dimensions.y / 2);

	targetToAssign.x = float(xDistr(generator));
	targetToAssign.y = float(yDistr(generator));
	
	pBlackboard->ChangeData("Checkpoint", targetToAssign);
	pBlackboard->ChangeData("ReachedCheckpoint", false);

	return Elite::Success;
}

inline Elite::BehaviorState SeekToTarget(Elite::Blackboard* pBlackboard)
{
	Elite::Vector2 target;
	Elite::Vector2 checkpoint;

	bool dataAvailable = pBlackboard->GetData("Target", target) && pBlackboard->GetData("Checkpoint", checkpoint);

	if (!dataAvailable)
	{
		return Elite::Failure;
	}

	target = checkpoint;

	pBlackboard->ChangeData("Target", target);
	pBlackboard->ChangeData("SteeringBehaviour", CurrentSteeringBehaviour::SEEK);

	return Elite::Success;
}

inline bool NewHouseWasFound(Elite::Blackboard* pBlackboard)
{
	bool foundHouse{};
	bool DataAvailable = pBlackboard->GetData("FoundNewHouse",foundHouse);
	if (!DataAvailable)
	{
		return false;
	}
	return foundHouse;
}

inline bool IsFacingEnemy(Elite::Blackboard* pBlackboard)
{
	Elite::Vector2 target;
	AgentInfo agent;
	IExamInterface* pInterface = nullptr;
	bool dataAvailable = pBlackboard->GetData("Agent", agent) && pBlackboard->GetData("Target", target) && pBlackboard->GetData("Interface",pInterface);
	if (!dataAvailable)
	{
		return false;
	}

	float agentRot = agent.Orientation;
	agentRot -= float(M_PI) / 2.f;
	Elite::Vector2 distanceToTarget = target - agent.Position;

	float cosRot = std::cos(agentRot);
	float sinRot = std::sin(agentRot);

	float ValueX = agent.FOV_Range * cosRot;
	float ValueY = agent.FOV_Range * sinRot;

	Elite::Vector2 LookAt(ValueX, ValueY);

	auto result = Elite::Dot(Elite::GetNormalized(distanceToTarget), Elite::GetNormalized(LookAt));

	pBlackboard->ChangeData("AngularVelocity", result);

	auto VrMagnitude = Elite::GetNormalized(LookAt).SqrtMagnitude();
	auto distanceMagnitude = Elite::GetNormalized(distanceToTarget).SqrtMagnitude();

	result /= (VrMagnitude * distanceMagnitude);
	

	//std::cout << "Result: " << result << std::endl;
	std::cout << "Result value: " << abs(result) << std::endl;

	if (abs(result) > 0.9976f)
	{
		return true;
	}
	else if (Elite::Distance(target,agent.Position) <= 5.5f)
	{
		if (abs(result) > 0.9776f)
		{
			return true;
		}
	}
	else if (Elite::Distance(target, agent.Position) <= 3.5f)
	{
		if (abs(result) > 0.7776f)
		{
			return true;
		}
	}
	

	return false;

}

inline Elite::BehaviorState FaceToEnemy(Elite::Blackboard* pBlackboard)
{
	CurrentSteeringBehaviour sB;
	bool dataAvailable = pBlackboard->GetData("SteeringBehaviour", sB);
	if (!dataAvailable)
	{
		return Elite::Failure;
	}
	sB = CurrentSteeringBehaviour::FACE;
	pBlackboard->ChangeData("SteeringBehaviour", sB);
	return Elite::Success;
}

inline Elite::BehaviorState EscapeHouse(Elite::Blackboard* pBlackboard)
{
	bool recentlyVisitedHouse{};
	bool isWandering{};
	CurrentSteeringBehaviour sB{};
	float currEscapeTimer{};
	float defaultEscapeTimer{};
	Elite::Vector2 target{};

	const bool dataAvailable = pBlackboard->GetData("SteeringBehaviour", sB)
		&& pBlackboard->GetData("RecentlyVisitedHouse", recentlyVisitedHouse)
		&& pBlackboard->GetData("Target", target)
		&& pBlackboard->GetData("CurrEscapeTimer", currEscapeTimer)
		&& pBlackboard->GetData("MaxEscapeTimer", defaultEscapeTimer)
		&& pBlackboard->GetData("IsWandering",isWandering);

	if (!dataAvailable)
	{
		return Elite::Failure;
	}

	if (!recentlyVisitedHouse)
	{
		return Elite::Failure;
	}

	pBlackboard->ChangeData("IsEscaping", true);

	if (!isWandering)
	{
		target.x -= 20.f;
		target.y += 20.f;


		if (currEscapeTimer >= defaultEscapeTimer)
		{
			recentlyVisitedHouse = false;
			pBlackboard->ChangeData("IsEscaping", false);
			pBlackboard->ChangeData("CurrEscapeTimer", 0.f);
		}

		sB = CurrentSteeringBehaviour::SEEK;
		pBlackboard->ChangeData("RecentlyVisitedHouse", recentlyVisitedHouse);
		pBlackboard->ChangeData("Target", target);
		pBlackboard->ChangeData("SteeringBehaviour", sB);
		return Elite::Success;
	}
	else
	{
		sB = CurrentSteeringBehaviour::WANDER;
		pBlackboard->ChangeData("SteeringBehaviour", sB);
		return Elite::Success;
	}
	

	
}

inline Elite::BehaviorState InspectNewHouse(Elite::Blackboard* pBlackboard)
{
	IExamInterface* m_pInterface{};
	HouseInfo house{};
	CurrentSteeringBehaviour sB{};
	Elite::Vector2 m_Target{};
	const bool DataAvailable = pBlackboard->GetData("SteeringBehaviour",sB) && pBlackboard->GetData("Interface",m_pInterface) && pBlackboard->GetData("NewHouse",house) && pBlackboard->GetData("Target",m_Target);
	if (!DataAvailable)
	{
		return Elite::Failure;
	}
	sB = CurrentSteeringBehaviour::SEEK;
	m_Target = Elite::Vector2{m_pInterface->NavMesh_GetClosestPathPoint(house.Center)};
	pBlackboard->ChangeData("Target",m_Target);
	pBlackboard->ChangeData("SteeringBehaviour", sB);
	return Elite::Success;
}

inline Elite::BehaviorState StartWander(Elite::Blackboard* pBlackboard)
{
	CurrentSteeringBehaviour sB;

	const bool dataAvailable = pBlackboard->GetData("SteeringBehaviour", sB);

	if (!dataAvailable)
	{
		return Elite::Failure;
	}

	sB = CurrentSteeringBehaviour::WANDER;
	pBlackboard->ChangeData("SteeringBehaviour", sB);
	return Elite::Success;
}

inline bool TooCloseToEnemy(Elite::Blackboard* pBlackboard)
{
	EntityInfo enemy;
	IExamInterface* interface;

	const bool DataAvailable = pBlackboard->GetData("Interface",interface) && pBlackboard->GetData("Enemy",enemy);
	if (!DataAvailable)
	{
		pBlackboard->ChangeData("CanRun",false);
		return false;
	}

	auto agentInfo = interface->Agent_GetInfo();

	if(abs(enemy.Location.x - agentInfo.Position.x) < abs(3.5f) &&
					abs(enemy.Location.y - agentInfo.Position.y) < abs(3.5f))
	{
		return true;
	}

	return false;
}

inline bool EnemiesInFOV(Elite::Blackboard* pBlackboard)
{
	bool HasEnemiesInFOV{false};
	bool DataAvailable = pBlackboard->GetData("EnemiesInFOV",HasEnemiesInFOV);
	if (!DataAvailable)
	{
		return false;
	}
	return HasEnemiesInFOV;
}

inline Elite::BehaviorState SeekToItem(Elite::Blackboard* pBlackboard)
{
	IExamInterface* m_pInterface{};
	std::vector<EntityInfo>* m_pItemMap{};
	std::vector<extendedHouseInfo>* m_pHouseMap{};
	CurrentSteeringBehaviour sB;
	Elite::Vector2 m_Target{};
	const bool DataAvailable = pBlackboard->GetData("SteeringBehaviour",sB) && pBlackboard->GetData("Interface",m_pInterface) && pBlackboard->GetData("ItemMap",m_pItemMap) && pBlackboard->GetData("Target",m_Target) && pBlackboard->GetData("HouseMap",m_pHouseMap);
	if (!DataAvailable)
	{
		return Elite::Failure;
	}

			Elite::Vector2 newTargetItem;
			bool firstCheckItem{};
			auto agentInfo = m_pInterface->Agent_GetInfo();
			for (EntityInfo value : (*m_pItemMap))
			{
				if (!firstCheckItem)
				{
					newTargetItem = value.Location;
					firstCheckItem = true;
				}

				else if(abs(Elite::Distance(value.Location,agentInfo.Position)) < abs(Elite::Distance(newTargetItem,agentInfo.Position)))
				{
					newTargetItem = value.Location;
				}
			}


			Elite::Vector2 newTarget;
			bool firstCheckHouse{};
			for (extendedHouseInfo value : *m_pHouseMap)
			{
				if (!firstCheckHouse && !value.RecentlyVisited)
				{
					newTarget = value.info.Center;
					firstCheckHouse = true;
				}


				else if(abs(Elite::Distance(value.info.Center,agentInfo.Position)) < abs(Elite::Distance(newTargetItem,agentInfo.Position)) && !value.RecentlyVisited)
				{
					newTarget = value.info.Center;
				}
			}


			if (!firstCheckHouse && !firstCheckItem)
			{
				return Elite::Failure;
			}

			if(abs(Elite::Distance(newTarget,agentInfo.Position)) < abs(Elite::Distance(newTargetItem,agentInfo.Position)) && firstCheckHouse && firstCheckItem)
			{
				m_Target = m_pInterface->NavMesh_GetClosestPathPoint(newTarget);
			}

			else if(firstCheckItem)
			{
				m_Target = m_pInterface->NavMesh_GetClosestPathPoint(newTargetItem);
			}
			else if(firstCheckHouse)
			{
				m_Target = m_pInterface->NavMesh_GetClosestPathPoint(newTarget);
			}


	//Use the navmesh to calculate the next navmesh point
	sB = CurrentSteeringBehaviour::SEEK;
	pBlackboard->ChangeData("SteeringBehaviour", sB);
	Elite::Vector2 nextTargetPos = Elite::Vector2{m_pInterface->NavMesh_GetClosestPathPoint(m_Target)};
	m_Target = nextTargetPos;
	pBlackboard->ChangeData("Target",m_Target);
	return  Elite::Success;
}


inline Elite::BehaviorState EvadeEnemy(Elite::Blackboard* pBlackboard)
{
	IExamInterface* m_pInterface{};
	CurrentSteeringBehaviour sB;
	float Radius{};
	float AngleChange{};
	const bool DataAvailable = pBlackboard->GetData("SteeringBehaviour",sB) && pBlackboard->GetData("Interface",m_pInterface) && pBlackboard->GetData("AngleChange",AngleChange) && pBlackboard->GetData("WanderRadius",Radius);

	if (!DataAvailable)
	{
		return Elite::Failure;
	}

	auto agentInfo = m_pInterface->Agent_GetInfo();


		sB = CurrentSteeringBehaviour::FLEE;
		pBlackboard->ChangeData("SteeringBehaviour", sB);
		pBlackboard->ChangeData("CanRun", true);
		pBlackboard->ChangeData("ReachedCheckpoint", true);
		return Elite::Success;

}

inline Elite::BehaviorState TryMedkit(Elite::Blackboard* pBlackboard)
{

	float Health{};
	std::vector<InventorySlotInfo>* m_pItemMap{};
	IExamInterface* m_pInterface{};
	bool DataAvailable = pBlackboard->GetData("Health",Health) && pBlackboard->GetData("Interface",m_pInterface) && pBlackboard->GetData("Inventory",m_pItemMap);

		for(int i = 0; i < int(m_pInterface->Inventory_GetCapacity()); ++i)
		{
			if ((*m_pItemMap)[i].InUse)
			{
				ItemInfo info{};
				m_pInterface->Inventory_GetItem(i,info);
				if (info.Type == eItemType::MEDKIT)
				{
				  m_pInterface->Inventory_UseItem(i);
				  m_pInterface->Inventory_RemoveItem(i);
				  (*m_pItemMap)[i].InUse = false;
				  return Elite::Success;
				}
			}

		}
	return Elite::Failure;
}

inline Elite::BehaviorState TryFood(Elite::Blackboard* pBlackboard)
{
	float Energy{};
	std::vector<InventorySlotInfo>* m_pItemMap{};
	IExamInterface* m_pInterface{};
	bool DataAvailable = pBlackboard->GetData("Energy",Energy) && pBlackboard->GetData("Interface",m_pInterface) && pBlackboard->GetData("Inventory",m_pItemMap);

		for(int i = 0; i < int(m_pInterface->Inventory_GetCapacity()); ++i)
		{
			if ((*m_pItemMap)[i].InUse)
			{
				ItemInfo info{};
				m_pInterface->Inventory_GetItem(i,info);
				if (info.Type == eItemType::FOOD)
				{
				  m_pInterface->Inventory_UseItem(i);
				  m_pInterface->Inventory_RemoveItem(i);
				  (*m_pItemMap)[i].InUse = false;
				  return Elite::Success;
				}
			}

		}
	return Elite::Failure;
}

inline Elite::BehaviorState TryGun(Elite::Blackboard* pBlackboard)
{
	std::vector<InventorySlotInfo>* m_pItemMap{};
	IExamInterface* m_pInterface{};
	bool DataAvailable = pBlackboard->GetData("Interface",m_pInterface) && pBlackboard->GetData("Inventory",m_pItemMap);

		for(int i = 0; i < int(m_pInterface->Inventory_GetCapacity()); ++i)
		{
			if ((*m_pItemMap)[i].InUse)
			{
				ItemInfo info{};
				m_pInterface->Inventory_GetItem(i,info);
				if (info.Type == eItemType::PISTOL)
				{
				  auto GunFired = m_pInterface->Inventory_UseItem(i);
					if (!GunFired)
					{
				    	m_pInterface->Inventory_RemoveItem(i);
				        (*m_pItemMap)[i].InUse = false;
					}
					pBlackboard->ChangeData("EnemiesInFOV",false);
				  return Elite::Success;
				}
			}

		} 
	return Elite::Failure;
}

inline bool ItemsInFOV(Elite::Blackboard* pBlackboard)
{
	bool HasItemsInFOV{false};
	bool DataAvailable = pBlackboard->GetData("ItemsInFOV",HasItemsInFOV);
	if (!DataAvailable)
	{
		return false;
	}
	return HasItemsInFOV;
}

inline bool HasSufficientEnergy(Elite::Blackboard* pBlackboard)
{

	float Energy{};
	bool DataAvailable = pBlackboard->GetData("Energy",Energy);
	if (!DataAvailable)
	{
		return false;
	}
	if (Energy < 5.0f)
	{
		return false;
	}
	
	return true;
};

inline bool IsInventoryFull(Elite::Blackboard* pBlackboard)
{
	bool IsFull{true};
	std::vector<InventorySlotInfo>* m_pItemMap{};
	const bool DataAvailable = pBlackboard->GetData("Inventory",m_pItemMap);

	if (!DataAvailable)
	{
		return false;
	}

	for (int i = 0; i < 5; ++i)
	{
		if (!(*m_pItemMap)[i].InUse)
		{
			IsFull = false;
		}
	}
	return IsFull;
}

inline bool HasItemInInventory(Elite::Blackboard* pBlackboard)
{
	std::vector<InventorySlotInfo>* m_pItemMap;
	const bool DataAvailable = pBlackboard->GetData("Inventory",m_pItemMap);

	if (!DataAvailable)
	{
		return false;
	}

	for(InventorySlotInfo info : *m_pItemMap)
	{
		if(info.InUse)
		{
			return true;
		}
	}

	return false;
}

inline bool HasItemInMap(Elite::Blackboard* pBlackboard)
{
	std::vector<EntityInfo>* vecEntities;
	bool dataAvailable = pBlackboard->GetData("ItemMap", vecEntities);
	if (!dataAvailable)
	{
		return false;
	}

	if (vecEntities->size() != 0)
	{
		return true;
	}
	return false;
}

inline bool BotInRangeOfItem(Elite::Blackboard* pBlackboard)
{
	bool IsInRange{false};
	const bool DataAvailable = pBlackboard->GetData("InRange",IsInRange);

	if (!DataAvailable)
	{
		return false;
	}

	return IsInRange;
}

inline bool HasGun(Elite::Blackboard* pBlackboard)
{
	std::vector<InventorySlotInfo>* m_pItemMap{};
	IExamInterface* m_pInterface{};
	bool DataAvailable = pBlackboard->GetData("Interface",m_pInterface) && pBlackboard->GetData("Inventory",m_pItemMap);
	if (!DataAvailable)
	{
		return false;
	}
	for(int i = 0; i < int(m_pInterface->Inventory_GetCapacity()); ++i)
		{
			if ((*m_pItemMap)[i].InUse)
			{
				ItemInfo info{};
				m_pInterface->Inventory_GetItem(i,info);
				if (info.Type == eItemType::PISTOL)
				{
					return true;
				}
			}
		}
	return false;
}

inline bool HasBuildingsToVisit(Elite::Blackboard* pBlackboard)
{
	bool hasCheckedAllFoundBuildings{false};
	std::vector<extendedHouseInfo>* m_pHouseMap{};
	const bool DataAvailable = pBlackboard->GetData("HouseMap",m_pHouseMap);
	if (!DataAvailable)
	{
		return false;
	}
	for(extendedHouseInfo value : *m_pHouseMap)
	{
		if (!value.RecentlyVisited)
		{
			hasCheckedAllFoundBuildings = true;
		}
	}

	return hasCheckedAllFoundBuildings;
}