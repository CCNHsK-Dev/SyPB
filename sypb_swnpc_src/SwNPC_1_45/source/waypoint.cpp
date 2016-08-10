
#include "core.h"

Waypoint::Waypoint(void)
{
	for (int i = 0; i < Const_MaxWaypoints; i++)
	{
		g_waypointPointFlag[i] = -1;
		g_waypointPointRadius[i] = -1.0f;
		g_waypointPointOrigin[i] = nullvec;
	}

	m_pathMatrix = null;
	m_distMatrix = null;
}

Waypoint::~Waypoint(void)
{
	if (m_pathMatrix != null)
		delete[] m_pathMatrix;

	if (m_distMatrix != null)
		delete[] m_distMatrix;

	m_pathMatrix = null;
	m_distMatrix = null;
}

void Waypoint::LoadWaypointData(Vector *origin, int32 *flags, float *radius, int16 **cnIndex, uint16 **cnFlags, int32 **cnDistance)
{
	int i, j, k;

	for (i = 0; i < Const_MaxWaypoints; i++)
	{
		g_waypointPointOrigin[i] = origin[i];
		g_waypointPointFlag[i] = flags[i];
		g_waypointPointRadius[i] = radius[i];

		for (j = 0; j < Const_MaxPathIndex; j++)
		{
			g_wpConnectionIndex[i][j] = cnIndex[i][j];
			g_wpConnectionFlags[i][j] = cnFlags[i][j];
			g_wpConnectionDistance[i][j] = cnDistance[i][j];
		}
	}

	if (m_distMatrix != null)
		delete[](m_distMatrix);

	if (m_pathMatrix != null)
		delete[] m_pathMatrix;

	m_distMatrix = null;
	m_pathMatrix = null;

	m_distMatrix = new int[g_numWaypoints * g_numWaypoints];
	m_pathMatrix = new int[g_numWaypoints * g_numWaypoints];

	if (m_distMatrix == null || m_pathMatrix == null)
		return;

	for (i = 0; i < g_numWaypoints; i++)
	{
		for (j = 0; j < g_numWaypoints; j++)
		{
			*(m_distMatrix + i * g_numWaypoints + j) = 999999;
			*(m_pathMatrix + i * g_numWaypoints + j) = -1;
		}
	}

	for (i = 0; i < g_numWaypoints; i++)
	{
		for (j = 0; j < Const_MaxPathIndex; j++)
		{
			if (g_wpConnectionIndex[i][j] >= 0 && g_wpConnectionIndex[i][j] < g_numWaypoints)
			{
				if (g_waypointPointFlag[i] & WAYPOINT_CROUCH ||
					g_waypointPointFlag[g_wpConnectionIndex[i][j]] & WAYPOINT_CROUCH)
					*(m_distMatrix + (i * g_numWaypoints) + g_wpConnectionIndex[i][j]) = (g_wpConnectionDistance[i][j] * 50);
				else
					*(m_distMatrix + (i * g_numWaypoints) + g_wpConnectionIndex[i][j]) = g_wpConnectionDistance[i][j];

				*(m_pathMatrix + (i * g_numWaypoints) + g_wpConnectionIndex[i][j]) = g_wpConnectionIndex[i][j];
			}
		}
	}

	for (i = 0; i < g_numWaypoints; i++)
		*(m_distMatrix + (i * g_numWaypoints) + i) = 0;

	for (k = 0; k < g_numWaypoints; k++)
	{
		for (i = 0; i < g_numWaypoints; i++)
		{
			for (j = 0; j < g_numWaypoints; j++)
			{
				if (*(m_distMatrix + (i * g_numWaypoints) + k) + *(m_distMatrix + (k * g_numWaypoints) + j) < (*(m_distMatrix + (i * g_numWaypoints) + j)))
				{
					*(m_distMatrix + (i * g_numWaypoints) + j) = *(m_distMatrix + (i * g_numWaypoints) + k) + *(m_distMatrix + (k * g_numWaypoints) + j);
					*(m_pathMatrix + (i * g_numWaypoints) + j) = *(m_pathMatrix + (i * g_numWaypoints) + k);
				}
			}
		}
	}
}

int Waypoint::FindNearest(Vector origin, float distance)
{
	int index = -1;
	for (int i = 0; i < g_numWaypoints; i++)
	{
		float thisDistance = GetDistance(origin, g_waypointPointOrigin[i]);
		if (thisDistance > distance)
			continue;

		index = i;
		distance = thisDistance;
	}

	return index;
}

int Waypoint::GetEntityWpIndex(edict_t *entity)
{
	int wpIndex = GetEntityWaypointPoint(entity);
	if (wpIndex >= 0 && wpIndex < g_numWaypoints)
		return wpIndex;

	return FindNearest(GetEntityOrigin(entity));
}

int Waypoint::GetEntityWpIndex(int id)
{
	return GetEntityWpIndex(INDEXENT(id));
}
