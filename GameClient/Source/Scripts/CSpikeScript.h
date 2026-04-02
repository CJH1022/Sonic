#pragma once
#include "CScript.h"

class CSpikeScript :
    public CScript
{
    private:
        
    public:
        virtual void Begin();
        virtual void Tick() override;

        void BeginOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider);

    public:

    public:
        CLONE(CSpikeScript);
        CSpikeScript();
        virtual ~CSpikeScript();
};
