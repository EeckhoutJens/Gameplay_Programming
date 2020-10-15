#pragma once
#include "IExamPlugin.h"
#include "Exam_HelperStructs.h"
#include "EliteAI/EliteBehaviorTree/EBehaviorTree.h"

class IBaseInterface;
class IExamInterface;
struct extendedHouseInfo
{
	HouseInfo info{};
	bool RecentlyVisited{ false };
	float TimeSinceLastVisit{ 0 };
};

struct InventorySlotInfo
{
	EntityInfo Iteminfo{};
	bool InUse{ false };
};

enum class CurrentSteeringBehaviour
{
	FLEE,
	SEEK,
	WANDER,
	FACE
};

class Plugin :public IExamPlugin
{
public:
	Plugin() {};
	virtual ~Plugin() {};

	void Initialize(IBaseInterface* pInterface, PluginInfo& info) override;
	void DllInit() override;
	void DllShutdown() override;

	void InitGameDebugParams(GameDebugParams& params) override;
	void Update(float dt) override;

	SteeringPlugin_Output UpdateSteering(float dt) override;
	void UpdateHouses(vector<HouseInfo>& vHouses, AgentInfo& agent, float& dt);
	void UpdateItems(AgentInfo& agent, vector<EntityInfo>& vEntitiesInFOV);
	void HandleEntities(vector<EntityInfo>& vEntitiesInFOV, AgentInfo& agent);
	void Render(float dt) const override;

private:
	//Interface, used to request data from/perform actions with the AI Framework
	IExamInterface* m_pInterface = nullptr;
	vector<HouseInfo> GetHousesInFOV() const;
	vector<EntityInfo> GetEntitiesInFOV() const;

	Elite::Vector2 m_Target = {};

	CurrentSteeringBehaviour m_currSteeringB{ CurrentSteeringBehaviour::WANDER };
	AgentInfo m_Agent{};
	HouseInfo m_MostRecentHouse{};
	EntityInfo m_CurrEnemy;
	bool m_IsSeekingToItem{ false };
	bool m_IsEscaping{ false };
	bool m_IsWanderingInHouse{ false };
	bool m_HasReachedCheckpoint{ true };
	float m_AngularVelocity{};
	const float m_MaxSeeAhead{15.f};
	bool RecentlyVisitedHouse{};
	float m_CurrFleeTimer{};
	const float m_DefaultFleeTimer{ 0.5f };
	const float m_DefaultEscapeTimer{ 2.5f };
	const float m_DefaultWanderInHouseTimer{ 2.5f };
	float m_CurrWanderTimer{};
	float m_CurrEscapeTimer{};
	Elite::Vector2 previousLocationEnemy{};
	Elite::Vector2 m_CheckpointLocation{};
	Elite::BehaviorTree* m_pBehaviorTree = nullptr;
	float m_WanderAngle = 0.f; //Internal
	EntityInfo m_EvadingEntity{};
	vector<extendedHouseInfo> m_VecHouses;
	vector<InventorySlotInfo> m_VecInventory;
	vector<EntityInfo> m_VecItems;
	bool m_FirstHouse{ false };
	float m_Radius = 2.f; //WanderRadius
	float m_CurrSeekTimer{};
	const float m_Offset = 6.f;
	float m_AngleChange = Elite::ToRadians(90); //Max WanderAngle change per frame
	const float m_ResetVisitedTimer{ 50 };

	bool m_CanRun = false; //Demo purpose
	bool m_GrabItem = false; //Demo purpose
	bool m_UseItem = false; //Demo purpose
	bool m_RemoveItem = false; //Demo purpose
	float m_AngSpeed = 0.f; //Demo purpose
};

//ENTRY
//This is the first function that is called by the host program
//The plugin returned by this function is also the plugin used by the host program
extern "C"
{
	__declspec (dllexport) IPluginBase* Register()
	{
		return new Plugin();
	}
}