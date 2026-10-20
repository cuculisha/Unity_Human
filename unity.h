#pragma once
#include <vector>
#include "men.h"
#include "Vec.h"

namespace Unity {
	class GameObject;
	class Component;
	class GameObjectManager;
	class Transform;
	class Camera;
	class MonoBehaviour;
	class SkinnedMeshRenderer;

	class Offset
	{
	public:
		static constexpr Ptr32 GOM_hash = 0xFDE740;
		static constexpr Ptr32 GOM = 0xFDE770;
		static constexpr Ptr32 typeTable = 0xFDE99C;
	};

	class GameObjectManager
	{
	public:
		Ptr32 sentinel1;
		Ptr32 list1Head;
		Ptr32 sentinel2;
		Ptr32 list2Head;

		Ptr32 addr = 0;

	public:
		static GameObjectManager Read(Ptr32 addr);
		static GameObject FindGameObj_Name(string ObjName, std::vector<GameObject> obj);
		static std::vector<GameObject> FindGameObj_Name_ALL(string ObjName, std::vector<GameObject> obj);
		static Ptr32 FindObjectByInstanceID(uint32_t instanceID, Ptr32 mgr);
		std::vector<GameObject> List_traverse_1();
		std::vector<GameObject> List_traverse_2();
		std::vector<GameObject> List_traverse_3();
	};

	class GameObject
	{
	public:
		char  pad0[0x1C];
		Ptr32 compArray;
		Ptr32 pad1;
		Ptr32 compCount;
		char     pad2[0x20];
		Ptr32 namePtr;

		Ptr32 addr;

	public:
		static GameObject Read(Ptr32 addr);
	    std::string GetName();
		std::string CompIdToName(int typeId);
	    Component FindComp_Id(int CompId);
	    Component FindComp_Name(string CompName);
		std::vector<Component> FindCompALL();
	};

	class Component
	{ 
	public:
		char pad0[0x08];
		Ptr32 internalData;
		char pad1[16];
		Ptr32 GameObjectPtr;
		char pad4[0x28];
		Ptr32 NamePtr;

		Ptr32 addr = 0; // 这里要赋值组件地址 才能正常读组件

	public:
		static Component Read(Ptr32 addr);
		GameObject GetGameObject();
		int GetTypeId();
		string GetTypeName();
		//模板要放在类里面
		template<typename T>
		T GetOffset(Ptr32 offset) {
			T TT = read<T>(addr + offset);
			return TT;
		}

		template<typename T>
		T CompToTTT() {
			T a;
			a.addr = this->addr;
			return a;
		}
	};
	// 因为继承Comp了 所以只能在方法里面读了
	class Transform : public Component
	{
	public:

		Ptr32 Get_Parent_array();
		Vec3 GetLocalPosition();
		Vec3 GetWorldPos();
		Vec3 GetLocalScale();
		Quat GetLocalRotation();
		int  GetParentIndex();
		Transform GetParent();
		Ptr32 GetArray();
		Ptr32 GetIdx();
		Ptr32 GetTransformDataEntry();
		int GetChildCount();
		Transform GetChild(int i);
		std::vector<Transform> GetChild();
		std::vector<Transform> GetChild_ALL();
	};

	class Camera : public Component
	{
	public:
		Mat4x4 GetWorldToCameraMatrix();
		Mat4x4 GetProjectionMatrix();

		Vec3 GetWorldPosition();

		bool WorldToScreen(Vec3 world, Vec2& outScreen, int screenW, int screenH);
		bool WorldToScreen(const Mat4x4& w2c, const Mat4x4& proj, Vec3 world, Vec2& outScreen, int screenW, int screenH);
	};

	class MonoBehaviour : public Component
	{
	public:
		std::string GetScriptName();
	};

	class SkinnedMeshRenderer : public Component
	{
	public:

		Ptr32 GetBonesArray();
		int GetBoneCount();
		Transform GetBoneTransform(int i);
	};

	inline Ptr32 GOM_hash = 0;
	inline GameObjectManager GOM{};
	inline Ptr32 typeTable = 0;

	struct GB_Data
	{
		string Name;
		GameObject GB;

		GB_Data() {}
		GB_Data(string name, GameObject gb) : Name(name), GB(gb) {}
	};

	struct CP_Data
	{
		string Name;
		Component Comp;

		CP_Data() { }
		CP_Data(string name, Component comp) : Name(name), Comp(comp) {}
	};

	struct UnityData
	{
		GB_Data GmObj;
		std::vector<CP_Data> Comp;
		UnityData(GB_Data GmObj, std::vector<CP_Data> Comp) : GmObj(GmObj), Comp(Comp) {}
	};

	inline std::vector<UnityData> GM;

	void Start();
	void Updata();
}