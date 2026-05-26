#pragma once

#include <Common/Common.hpp>
#include <CS2/SDK/Math/QAngle.hpp>
#include <CS2/SDK/Math/Vector3.hpp>

class CCSGOInput;
class CUserCmd;
class C_CSPlayerPawn;
struct BestTarget_t;

class IAimbot
{
public:
	virtual void OnCreateMove( CCSGOInput* pInput , CUserCmd* pUserCmd ) = 0;
};

class CAimbot final : public IAimbot
{
public:
	virtual void OnCreateMove( CCSGOInput* pInput , CUserCmd* pUserCmd ) override;

private:
	// Walks entity cache once and returns best target + cached bone position
	auto FindBestTarget( const QAngle& viewAngles ) -> BestTarget_t;

	// Returns world-space bone position for the configured aim bone
	auto GetBonePosition( C_CSPlayerPawn* pPawn ) -> Vector3;

	// Returns angular distance (degrees) from viewAngles to a world-space position
	auto GetFovToTarget( const QAngle& viewAngles , const Vector3& bonePos ) -> float;

	// Full validity check: alive, enemy/team filter, visible filter
	auto IsValidTarget( C_CSPlayerPawn* pPawn ) -> bool;
};

auto GetAimbot() -> CAimbot*;
