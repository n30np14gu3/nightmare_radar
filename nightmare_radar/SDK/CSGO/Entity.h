#pragma once
#include "../math_tools.h"

typedef struct _KVec
{
	float x;
	float y;
	float z;
}KVec;

class Entity
{
public:
	Entity(void* ptr, DWORD32 engine);
	Entity(void* ptr, DWORD32 engine, DWORD32 id);
	Vector getOrigin();
	QAngle getViewAngles();
	bool isDormant();
	DWORD32 getTeam();
	DWORD32 getHealth();
	bool isValid();

private:
	DWORD32 m_iId;
	DWORD32 m_dwAddress;
	DWORD32 m_dwEngine;
	void* m_kDriver;
};
