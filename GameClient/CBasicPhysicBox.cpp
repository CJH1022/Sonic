#include "pch.h"
#include "CBasicPhysicBox.h"


CBasicPhysicBox::CBasicPhysicBox()
{
}

CBasicPhysicBox::~CBasicPhysicBox()
{
}

void CBasicPhysicBox::Begin()
{
	ADD_DYNAMIC_BEGIN_OVERLAP(CBasicPhysicBox::BeginOverlap);
}

void CBasicPhysicBox::Tick()
{
}

void CBasicPhysicBox::BeginOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider)
{

}