#include <string>
#include "unity.h"
#include "men.h"
#include "lib/xxhash.h"

using namespace Unity;

GameObjectManager GameObjectManager::Read(Ptr32 addr) {
    GameObjectManager GOM = read<GameObjectManager>(addr);
	GOM.addr = addr;
    return GOM;
}

GameObject GameObjectManager::FindGameObj_Name(string ObjName, std::vector<GameObject> obj) {
	for (auto& o : obj) {
		if (o.GetName() == ObjName) {
			return o;
		}
	}
	return GameObject{};
}

Ptr32 GameObjectManager::FindObjectByInstanceID(uint32_t instanceID, Ptr32 goh)
{
    Ptr32 tableBase = read<Ptr32>(goh);
    Ptr32 mask = read<Ptr32>(goh + 0x4);

    Ptr32 h = XXH32_hashFromId(instanceID);
    /*                                               //hash算法
    int32_t mix0 = 4097 * id + 2127912214;
    uint32_t mix1 = mix0 ^ (mix0 >> 19) ^ 0xC761C23C;

    uint32_t mix2 = mix1 + 374761393 + 32 * mix1;
    uint32_t mix3 = (mix2 << 9) ^ (mix2 - 744332180);

    uint32_t mix4 = mix3 + 8 * mix3 - 42973499;
    uint32_t hash = mix4 ^ (mix4 >> 16) ^ 0xB55A4F09;
    */

    Ptr32 slot = h & mask;
    Ptr32 alignedHash = h & 0xFFFFFFFC;

    Ptr32 probeOffset = 4;
    int guard = 0;

    while (guard++ < 10000) {
        Ptr32 slotAddr = tableBase + 3 * slot;
        Ptr32 slotHash = read<Ptr32>(slotAddr + 0);
        Ptr32 slotID = read<Ptr32>(slotAddr + 4);

        if (slotHash == 0xFFFFFFFF) return 0;

        if (slotHash == alignedHash && slotID == instanceID) {
            return read<Ptr32>(slotAddr + 8);
        }
        slot = (slot + probeOffset) & mask;
        probeOffset += 4;
    }
    return 0;
}

std::vector<GameObject> GameObjectManager::FindGameObj_Name_ALL(string ObjName, std::vector<GameObject> obj) {
    std::vector<GameObject> this_obj;
    for (auto& o : obj) {
        if (o.GetName() == ObjName) {
            this_obj.push_back(o);
        }
    }
    return this_obj;
}

std::vector<GameObject> GameObjectManager::List_traverse_1()
{
    std::vector<GameObject> GameObjects;
    Ptr32 node = GOM.list1Head;
    int count = 0;
    while (GOM.sentinel1 != node && count++ < 10000)
    {
        Ptr32 objAddr = read<Ptr32>(node + 0x8);
        if (objAddr != 0)
        {
            GameObject obj = GameObject::Read(objAddr);
            GameObjects.push_back(obj);
        }
        node = read<Ptr32>(node + 0x4);
    }
    return GameObjects;
}

std::vector<GameObject> GameObjectManager::List_traverse_2()
{
    std::vector<GameObject> GameObjects;
    Ptr32 node = GOM.list2Head;
    int count = 0;
    while (GOM.sentinel2 != node && count++ < 10000)
    {
        Ptr32 objAddr = read<Ptr32>(node + 0x8);
        if (objAddr != 0)
        {
            GameObject obj = GameObject::Read(objAddr);
            GameObjects.push_back(obj);
        }
        node = read<Ptr32>(node + 0x4);
    }
    return GameObjects;
}

GameObject GameObject::Read(Ptr32 addr) {
    GameObject GO = read<GameObject>(addr);
    GO.addr = addr;
    return GO;
}

std::string GameObject::GetName() {
	char buf[128] = {};
	if (namePtr) read_str(namePtr, buf, sizeof(buf));
	return buf;
}

std::string GameObject::CompIdToName(int typeId) {
    uint32_t typeDesc = read<uint32_t>(typeTable + typeId * 0x4);
    if (!typeDesc) return "";

    uint32_t namePtr = read<uint32_t>(typeDesc + 0x8);
    if (!namePtr) return "";

    char buf[64] = {};
    read_str(namePtr, buf, sizeof(buf));
    return buf;
}

Component GameObject::FindComp_Id(int CompId) {
    for (Ptr32 i = 0; i < compCount; i++) {
        Ptr32 M = compArray + i * 0x8;
        if (read<Ptr32>(M) == CompId) {
			Component c;
			c.addr = read<Ptr32>(M + 0x4);
            if (c.addr != 0) return c;
        }
    }
    return Component{};
}

Component GameObject::FindComp_Name(string compName) {
    for (uint32_t i = 0; i < compCount; i++) {
        Ptr32 M = compArray + i * 0x8;
        Ptr32 typeId = read<Ptr32>(M);
        Ptr32 comp = read<Ptr32>(M + 0x4);

        if (comp == 0) continue;

        std::string typeName = GameObject::CompIdToName(typeId);
        if (typeName == compName) {
            Component Comp;
            Comp.addr = comp;
            return Comp;
        }
    }
    return Component{};
}

std::vector<Component> GameObject::FindCompALL() {
    std::vector<Component> Comps;
	Comps.resize(compCount);
    for (uint32_t i = 0; i < compCount; i++) {
        Ptr32 M = compArray + i * 0x8;
        Ptr32 typeId = read<Ptr32>(M);
        Ptr32 comp = read<Ptr32>(M + 0x4);

        if (comp == 0) continue;
        Comps[i].addr = comp;
    }
    return Comps;
}

GameObject Component::GetGameObject(){
    return GameObject::Read(read<Ptr32>(addr + 0x1C));
}

Component Component::Read(Ptr32 addr) {
    Component Comp = read<Component>(addr);
	Comp.addr = addr;
    return Comp;
}

int Component::GetTypeId() {
    GameObject GO = this->GetGameObject();

    for (uint32_t i = 0; i < GO.compCount; i++) {
        Ptr32 M = GO.compArray + i * 8;
        Ptr32 comp = read<Ptr32>(M + 4);
        if (comp == addr) {
            return read<int>(M);
        }
    }
    return 0;
}

std::string Component::GetTypeName()
{
    int typeId = this->GetTypeId();
    Ptr32 typeDesc = read<Ptr32>(typeTable + typeId * 4);
    Ptr32 namePtr = read<Ptr32>(typeDesc + 0x8);

    char buf[64] = {};
    read_str(namePtr, buf, sizeof(buf));
    return buf;
}

Ptr32 Transform::GetArray() {
    Ptr32 internalData = read<Ptr32>(addr + 0x20);
    return read<Ptr32>(internalData + 0x8);
}

Ptr32 Transform::GetIdx() {
    return read<Ptr32>(addr + 0x24);
}

Ptr32 Transform::Get_Parent_array() {
    if (!this->addr) return -1;
    Ptr32 internalData = read<Ptr32>(this->addr + 0x20);
    return read<Ptr32>(internalData + 0xC);
}

Ptr32 Transform::GetTransformDataEntry() {
    Ptr32 array = GetArray();
    Ptr32 index = GetIdx();
    return array + 0x30 * index;
}

Vec3 Transform::GetLocalPosition() {
    Ptr32 M = GetTransformDataEntry();
    if (!M) return {};
    return read<Vec3>(M);
}

Quat Transform::GetLocalRotation() {
    Quat q{};
    Ptr32 M = GetTransformDataEntry();
    if (!M) return q;
    return read<Quat>(M + 0x10);
}

Vec3 Transform::GetLocalScale() {
    Ptr32 M = GetTransformDataEntry();
    if (!M) return {};
    return read<Vec3>(M + 0x20);
}

int Transform::GetParentIndex() {
    Ptr32 parentArray = Get_Parent_array();
    if (!parentArray) return -1;

    Ptr32 index = GetIdx();
    return read<int>(parentArray + 0x4 * index);
}

Vec3 Transform::GetWorldPos() {
    Ptr32 arrayBase = this->GetArray();
    Ptr32 parentArray = this->Get_Parent_array();
    if (!arrayBase || !parentArray) return {};

    Ptr32 index = GetIdx();
    if (index > 100000) return {};

    Ptr32 entry = arrayBase + 0x30 * index;
    Vec3 world = read<Vec3>(entry);

    int parentIdx = read<int>(parentArray + 4 * index);
    int depth = 0;

    while (parentIdx >= 0 && depth < 64) {
        Ptr32 pX = arrayBase + 0x30 * parentIdx;

        Vec3 p = read<Vec3>(pX + 0x00);
        Quat r = read<Quat>(pX + 0x10);
        Vec3 s = read<Vec3>(pX + 0x20);

        world.x *= s.x; world.y *= s.y; world.z *= s.z;

        float nx, ny, nz;
        quat_rotate(r.x, r.y, r.z, r.w, world.x, world.y, world.z, &nx, &ny, &nz);

        world.x = nx; world.y = ny; world.z = nz;
        world.x += p.x; world.y += p.y; world.z += p.z;

        parentIdx = read<int>(parentArray + 4 * parentIdx);
        depth++;
    }
    return world;
}

Mat4x4 Camera::GetWorldToCameraMatrix() {
    return read_mat4x4(addr + 0x24);
}

Mat4x4 Camera::GetProjectionMatrix() {
    return read_mat4x4(addr + 0x64);
}

Vec3 Camera::GetWorldPosition() {
    Mat4x4 w2c = GetWorldToCameraMatrix();
    float tx = w2c.m[12], ty = w2c.m[13], tz = w2c.m[14];

    Vec3 p;
    p.x = -(w2c.m[0] * tx + w2c.m[1] * ty + w2c.m[2] * tz);
    p.y = -(w2c.m[4] * tx + w2c.m[5] * ty + w2c.m[6] * tz);
    p.z = -(w2c.m[8] * tx + w2c.m[9] * ty + w2c.m[10] * tz);
    return p;
}

bool Camera::WorldToScreen(Vec3 world, Vec2& outScreen, int screenW, int screenH)
{
    Mat4x4 w2c = Camera::GetWorldToCameraMatrix();
    Mat4x4 proj = Camera::GetProjectionMatrix();

    float vx = w2c.m[0] * world.x + w2c.m[4] * world.y + w2c.m[8] * world.z + w2c.m[12];
    float vy = w2c.m[1] * world.x + w2c.m[5] * world.y + w2c.m[9] * world.z + w2c.m[13];
    float vz = w2c.m[2] * world.x + w2c.m[6] * world.y + w2c.m[10] * world.z + w2c.m[14];
    float vw = w2c.m[3] * world.x + w2c.m[7] * world.y + w2c.m[11] * world.z + w2c.m[15];

    if (vz >= -0.001f) return false;

    float cx = proj.m[0] * vx + proj.m[4] * vy + proj.m[8] * vz + proj.m[12] * vw;
    float cy = proj.m[1] * vx + proj.m[5] * vy + proj.m[9] * vz + proj.m[13] * vw;
    float cw = proj.m[3] * vx + proj.m[7] * vy + proj.m[11] * vz + proj.m[15] * vw;

    float cwAbs = fabsf(cw);
    if (cwAbs < 0.001f) return false;

    float ndcX = cx / cwAbs;
    float ndcY = cy / cwAbs;

    outScreen.x = (ndcX * 0.5f + 0.5f) * (float)screenW;
    outScreen.y = (1.0f - (ndcY * 0.5f + 0.5f)) * (float)screenH;
    return true;
}

bool Camera::WorldToScreen(const Mat4x4& w2c, const Mat4x4& proj, Vec3 world, Vec2& outScreen, int screenW, int screenH)
{
    const float* w = reinterpret_cast<const float*>(&w2c);
    const float* p = reinterpret_cast<const float*>(&proj);

    float vx = w[0] * world.x + w[4] * world.y + w[8] * world.z + w[12];
    float vy = w[1] * world.x + w[5] * world.y + w[9] * world.z + w[13];
    float vz = w[2] * world.x + w[6] * world.y + w[10] * world.z + w[14];
    float vw = w[3] * world.x + w[7] * world.y + w[11] * world.z + w[15];

    float cx = p[0] * vx + p[4] * vy + p[8] * vz + p[12] * vw;
    float cy = p[1] * vx + p[5] * vy + p[9] * vz + p[13] * vw;
    float cw = p[3] * vx + p[7] * vy + p[11] * vz + p[15] * vw;

    if (cw < 0.001f) return false;

    float ndcX = cx / cw;
    float ndcY = cy / cw;

    outScreen.x = (ndcX * 0.5f + 0.5f) * screenW;
    outScreen.y = (1.0f - (ndcY * 0.5f + 0.5f)) * screenH;
    return true;
}

std::string MonoBehaviour::GetScriptName() {
    Ptr32 VVV = read<Ptr32>(addr + 0x44);
    if (!VVV) return "";

    Ptr32 MonoClass = read<Ptr32>(VVV + 0x4);
    if (!MonoClass) return "";

    Ptr32 MamePtr = read<Ptr32>(MonoClass + 0x2C);
    if (!MamePtr) return "";

    char buf[128] = {};
    read_str(MamePtr, buf, sizeof(buf));
    return buf;
}

Ptr32 SkinnedMeshRenderer::GetBonesArray() {
    return read<Ptr32>(addr + 0x15C);
}

int SkinnedMeshRenderer::GetBoneCount() {
    return read<int>(addr + 0x164);
}

Transform SkinnedMeshRenderer::GetBoneTransform(int i) {
    Transform t;
    if (i < 0) return t;

    Ptr32 arr = GetBonesArray();
    if (!arr) return t;

    uint32_t boneID = read<uint32_t>(arr + i * 4);
    if (!boneID) return t;

    uint32_t boneAddr = GameObjectManager::FindObjectByInstanceID(boneID, GOM_hash);
    t.addr = boneAddr;
    return t;
}

int Transform::GetChildCount() {
    return read<int>(addr + 0x58);
}

Transform Transform::GetChild(int i) {
    Transform t;
    int c = GetChildCount();

    Ptr32 arr = read<Ptr32>(addr + 0x50);
    t.addr = read<Ptr32>(arr + i * 4);
    return t;
}

std::vector<Transform> Transform::GetChild() {
    std::vector<Transform> t{};
    if (!addr) return t;

    int c = GetChildCount();

    Ptr32 arr = read<Ptr32>(addr + 0x50);
    for (int i = 0; i < c; i++) {
        Ptr32 childAddr = read<Ptr32>(arr + i * 4);
        if (childAddr) {
            Transform c;
            c.addr = childAddr;
            t.push_back(c);
        }
    }
    return t;
}

std::vector<Transform> Transform::GetChild_ALL() {
    std::vector<Transform> t{};
    if (!addr) return t;

    int c = GetChildCount();
    if (c <= 0) return t;

    Ptr32 arr = read<Ptr32>(addr + 0x50);
    for (int i = 0; i < c; i++) {
        Ptr32 childAddr = read<Ptr32>(arr + i * 4);
        if (!childAddr) continue;

        Transform child;
        child.addr = childAddr;
        t.push_back(child);

        std::vector<Transform> sub = child.GetChild_ALL();
        t.insert(t.end(), sub.begin(), sub.end());
    }
    return t;
}

Transform Transform::GetParent() {
    Transform p;
    if (!addr) return p;
    p.addr = read<Ptr32>(addr + 0x60);
    return p;
}
