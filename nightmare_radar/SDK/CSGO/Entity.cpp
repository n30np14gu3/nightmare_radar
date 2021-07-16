#include <dwmapi.h>
#include <Windows.h>
#include <SubAuth.h>
#include <comdef.h>

#include "Entity.h"
#include "../globals.h"

#include "../../ring0/kernel_interface.h"

Entity::Entity(void* ptr, DWORD32 engine)
{
	m_iId = 1;
	m_dwAddress = 0;
	m_kDriver = ptr;
	m_dwEngine = engine;

	KernelInterface* ring0 = static_cast<KernelInterface*>(m_kDriver);
	ring0->Read32(ring0->Modules->bClient + dwLocalPlayer, &m_dwAddress);
}

Entity::Entity(void* ptr, DWORD32 engine, DWORD32 id)
{
	m_iId = id;
	m_dwAddress = 0;
	m_kDriver = ptr;
	m_dwEngine = engine;

	KernelInterface* ring0 = static_cast<KernelInterface*>(m_kDriver);

	ring0->Read32(ring0->Modules->bClient + dwEntityList + (m_iId - 1) * 0x10, &m_dwAddress);
}

Vector Entity::getOrigin()
{
	Vector result(0, 0, 0);
	KernelInterface* ring0 = static_cast<KernelInterface*>(m_kDriver);
	ring0->Read32(m_dwAddress + m_vecOrigin, &result);
	return result;
}

QAngle Entity::getViewAngles()
{
	QAngle result(0, 0, 0);
	KernelInterface* ring0 = static_cast<KernelInterface*>(m_kDriver);
	ring0->Read32(m_dwEngine + m_vecViewAngles, &result);
	return result;
}

bool Entity::isDormant()
{
	bool result = false;
	KernelInterface* ring0 = static_cast<KernelInterface*>(m_kDriver);
	ring0->Read32(m_dwAddress + m_bDormant, &result);
	return result;
}

DWORD32 Entity::getTeam()
{
	DWORD32 result = 0;
	KernelInterface* ring0 = static_cast<KernelInterface*>(m_kDriver);
	ring0->Read32(m_dwAddress + m_iTeamNum, &result);
	return result;
}

DWORD32 Entity::getHealth()
{
	DWORD32 result = 0;
	KernelInterface* ring0 = static_cast<KernelInterface*>(m_kDriver);
	ring0->Read32(m_dwAddress + m_iHealth, &result);
	return result;
}

bool Entity::isValid()
{
	return m_dwAddress != 0;
}
