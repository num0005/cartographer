#include <stdafx.h>
#include "RenderDebug.h"
#include <d3d9.h>
#include "Patches.h"
#include "ScenarioStructureBSP.h"

scenario_structure_bsp_block *get_sbsp()
{
	return *reinterpret_cast<scenario_structure_bsp_block **>(0x879E6C);
}

IDirect3DDevice9Ex *get_global_d3d_device()
{
	return *reinterpret_cast<IDirect3DDevice9Ex **>(0xE3C6B4);
}

struct s_debug_vertex
{
	real_point3d point;
	D3DCOLOR colour;
};
CHECK_STRUCT_SIZE(s_debug_vertex, 0x10);

inline D3DCOLOR halo_colour_to_d3d_colour(const colour_rgba *colour)
{
	auto halo_to_hex = [](float num) -> int { return int(num * 255); };

	return D3DCOLOR_ARGB(halo_to_hex(colour->alpha), halo_to_hex(colour->red), halo_to_hex(colour->green), halo_to_hex(colour->blue));
}

void draw_debug_line(real_point3d *v0, real_point3d *v1, 
	const colour_rgba *start_colour = nullptr, 
	const colour_rgba *end_colour = nullptr)
{
	const static colour_rgba white;
	if (!start_colour)
	{
		start_colour = &white;
	}

	if (!end_colour)
	{
		end_colour = start_colour;
	}

	s_debug_vertex line_info[2];
	line_info[0].point = *v0;
	line_info[0].colour = halo_colour_to_d3d_colour(start_colour);
	line_info[1].point = *v1;
	line_info[1].colour = halo_colour_to_d3d_colour(end_colour);


	LOG_CHECK(get_global_d3d_device()->DrawPrimitiveUP(D3DPT_LINELIST, 1, line_info, sizeof(s_debug_vertex)) == NOERROR);
}

const colour_rgba pathfinding_debug_colour_default( 1.0f, 1.0f, 0.5f, 0.5f );

char game_in_progress()
{
	char result; // al@1
	char *game_globals = *reinterpret_cast<char**>(0x882D3C);
	result = 0;
	if (game_globals)
	{
		if (*(BYTE *)(game_globals + 0x1190))
			result = 1;
	}
	return result;
}

void __cdecl render_debug_info_game_in_progress()
{
	if (game_in_progress())
	{
		auto bsp = get_sbsp();
		if (bsp)
		{
			auto pathfinding = bsp->pathfindingData[0];
			if (pathfinding)
			{
				float zBias = 0.09f; // your 'offset' value. Play with it until you get good results
				get_global_d3d_device()->SetRenderState(D3DRS_DEPTHBIAS, *(DWORD *)&zBias);
				get_global_d3d_device()->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
				//get_global_d3d_device()->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);

				colour_rgba pathfinding_debug_colour = pathfinding_debug_colour_default;
				for (const auto &current_link : pathfinding->links)
				{
					auto vertex1 = pathfinding->vertices[current_link.vertex1];
					auto vertex2 = pathfinding->vertices[current_link.vertex2];

					if (vertex1 && vertex1)
						draw_debug_line(&vertex1->point, &vertex2->point, &pathfinding_debug_colour);
				}
			}
		}
	}
}
