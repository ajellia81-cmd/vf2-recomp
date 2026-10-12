// license:BSD-3-Clause
// copyright-holders:Ville Linde, Aaron Giles
// Polygon scan conversion: MAME's poly_manager (devices/video/poly.h, BSD-3-Clause,
// copyright-holders: Ville Linde, Aaron Giles) without its work queue. MAME computes a
// primitive's extents when it's submitted and runs the scanline callbacks later, in submission
// order for any one scanline; here the callbacks run at once, which draws the same pixels.
// Only what the Model 2 renderer uses: render_triangle and render_polygon, which can also be
// given the primitive's object data (several threads drawing at once, each its own rows).
#pragma once
#include "mame_lite.h"
#include <array>
#include <climits>
#include <functional>
#include <limits>

template<typename BaseType, class ObjectType, int MaxParams>
class poly_manager
{
public:
	// each vertex has an X/Y coordinate and a set of parameters
	struct vertex_t
	{
		vertex_t() { }
		vertex_t(BaseType _x, BaseType _y) { x = _x; y = _y; }

		BaseType x, y;                          // X, Y coordinates
		std::array<BaseType, MaxParams> p;      // iterated parameters
	};

	// a single extent describes a span and a list of parameter extents
	struct extent_t
	{
		struct param_t
		{
			BaseType start;                     // parameter value at start
			BaseType dpdx;                      // dp/dx relative to start
		};
		int16_t startx, stopx;                  // starting (inclusive)/ending (exclusive) endpoints
		std::array<param_t, MaxParams> param;   // array of parameter start/delays
		void *userdata;                         // custom per-span data
	};

	using render_delegate = std::function<void (int32_t, extent_t const &, ObjectType const &, int)>;

	// the object data of the primitive being submitted (MAME keeps an array of them)
	struct object_holder
	{
		ObjectType obj{};
		ObjectType &next() { obj = ObjectType{}; return obj; }
		ObjectType &last() { return obj; }
	};
	object_holder &object_data() { return m_object; }
	void wait(char const * = "general") {}

	template<int ParamCount>
	uint32_t render_triangle(rectangle const &cliprect, const render_delegate &callback, vertex_t const &v1, vertex_t const &v2, vertex_t const &v3, ObjectType const *object = nullptr);

	template<int NumVerts, int ParamCount>
	uint32_t render_polygon(rectangle const &cliprect, const render_delegate &callback, vertex_t const *v, ObjectType const *object = nullptr);

private:
	object_holder m_object;

	// round in a cross-platform consistent manner
	static int32_t round_coordinate(BaseType value)
	{
		// saturate (avoid overflow/underflow)
		if (value >= BaseType(std::numeric_limits<int32_t>::max()))
			return std::numeric_limits<int32_t>::max();
		const BaseType ipart = std::floor(value);
		if (ipart < BaseType(std::numeric_limits<int32_t>::min()))
			return std::numeric_limits<int32_t>::min();

		// round
		// TODO: this rounds the midpoint towards negative infinity - should it use more standard behaviour?
		const BaseType fpart = value - ipart;
		return int32_t(ipart) + ((fpart > BaseType(0.5)) ? 1 : 0);
	}
};


//-------------------------------------------------
//  render_triangle - render a single triangle
//  given 3 vertexes
//-------------------------------------------------

template<typename BaseType, class ObjectType, int MaxParams>
template<int ParamCount>
uint32_t poly_manager<BaseType, ObjectType, MaxParams>::render_triangle(const rectangle &cliprect, const render_delegate &callback, const vertex_t &_v1, const vertex_t &_v2, const vertex_t &_v3, ObjectType const *object)
{
	ObjectType const &obj = object ? *object : m_object.last();
	vertex_t const *v1 = &_v1;
	vertex_t const *v2 = &_v2;
	vertex_t const *v3 = &_v3;

	// first sort by Y
	if (v2->y < v1->y)
		std::swap(v1, v2);
	if (v3->y < v2->y)
	{
		std::swap(v2, v3);
		if (v2->y < v1->y)
			std::swap(v1, v2);
	}

	// compute some integral X/Y vertex values
	int32_t v1y = round_coordinate(v1->y);
	int32_t v3y = round_coordinate(v3->y);

	// clip coordinates
	int32_t v1yclip = v1y;
	int32_t v3yclip = v3y;
	v1yclip = std::max(v1yclip, cliprect.min_y);
	v3yclip = std::min(v3yclip, cliprect.max_y + 1);
	if (v3yclip - v1yclip <= 0)
		return 0;

	// compute the slopes for each portion of the triangle
	BaseType dxdy_v1v2 = (v2->y == v1->y) ? BaseType(0.0) : (v2->x - v1->x) / (v2->y - v1->y);
	BaseType dxdy_v1v3 = (v3->y == v1->y) ? BaseType(0.0) : (v3->x - v1->x) / (v3->y - v1->y);
	BaseType dxdy_v2v3 = (v3->y == v2->y) ? BaseType(0.0) : (v3->x - v2->x) / (v3->y - v2->y);

	// compute parameter starting points and deltas
	std::array<BaseType, ParamCount> param_start;
	std::array<BaseType, ParamCount> param_dpdx;
	std::array<BaseType, ParamCount> param_dpdy;
	if (ParamCount > 0)
	{
		BaseType a00 = v2->y - v3->y;
		BaseType a01 = v3->x - v2->x;
		BaseType a02 = v2->x*v3->y - v3->x*v2->y;
		BaseType a10 = v3->y - v1->y;
		BaseType a11 = v1->x - v3->x;
		BaseType a12 = v3->x*v1->y - v1->x*v3->y;
		BaseType a20 = v1->y - v2->y;
		BaseType a21 = v2->x - v1->x;
		BaseType a22 = v1->x*v2->y - v2->x*v1->y;
		BaseType det = a02 + a12 + a22;

		if (std::abs(det) < BaseType(0.00001))
		{
			for (int paramnum = 0; paramnum < ParamCount; paramnum++)
			{
				param_dpdx[paramnum] = BaseType(0.0);
				param_dpdy[paramnum] = BaseType(0.0);
				param_start[paramnum] = v1->p[paramnum];
			}
		}
		else
		{
			BaseType idet = BaseType(1.0) / det;
			for (int paramnum = 0; paramnum < ParamCount; paramnum++)
			{
				param_dpdx[paramnum]  = idet * (v1->p[paramnum]*a00 + v2->p[paramnum]*a10 + v3->p[paramnum]*a20);
				param_dpdy[paramnum]  = idet * (v1->p[paramnum]*a01 + v2->p[paramnum]*a11 + v3->p[paramnum]*a21);
				param_start[paramnum] = idet * (v1->p[paramnum]*a02 + v2->p[paramnum]*a12 + v3->p[paramnum]*a22);
			}
		}
	}

	// compute the X extents for each scanline
	int32_t pixels = 0;
	extent_t extent;
	for (int32_t curscan = v1yclip; curscan < v3yclip; curscan++)
	{
		// compute the ending X based on which part of the triangle we're in
		BaseType fully = BaseType(curscan) + BaseType(0.5);
		BaseType startx = v1->x + (fully - v1->y) * dxdy_v1v3;
		BaseType stopx;
		if (fully < v2->y)
			stopx = v1->x + (fully - v1->y) * dxdy_v1v2;
		else
			stopx = v2->x + (fully - v2->y) * dxdy_v2v3;

		// clamp to full pixels
		int32_t istartx = round_coordinate(startx);
		int32_t istopx = round_coordinate(stopx);

		// force start < stop
		if (istartx > istopx)
			std::swap(istartx, istopx);

		// apply left/right clipping
		istartx = std::max(istartx, cliprect.min_x);
		istopx = std::min(istopx, cliprect.max_x + 1);

		// set the extent and update the total pixel count
		if (istartx >= istopx)
			istartx = istopx = 0;
		extent.startx = istartx;
		extent.stopx = istopx;
		pixels += istopx - istartx;

		// fill in the parameters for the extent
		BaseType fullstartx = BaseType(istartx) + BaseType(0.5);
		for (int paramnum = 0; paramnum < ParamCount; paramnum++)
		{
			extent.param[paramnum].start = param_start[paramnum] + fullstartx * param_dpdx[paramnum] + fully * param_dpdy[paramnum];
			extent.param[paramnum].dpdx = param_dpdx[paramnum];
		}
		callback(curscan, extent, obj, 0);
	}
	return pixels;
}


//-------------------------------------------------
//  render_polygon - render a single polygon up
//  to 32 vertices
//-------------------------------------------------

template<typename BaseType, class ObjectType, int MaxParams>
template<int NumVerts, int ParamCount>
uint32_t poly_manager<BaseType, ObjectType, MaxParams>::render_polygon(rectangle const &cliprect, const render_delegate &callback, vertex_t const *v, ObjectType const *object)
{
	ObjectType const &obj = object ? *object : m_object.last();
	// determine min/max Y vertices
	BaseType minx = v[0].x;
	BaseType maxx = v[0].x;
	int minv = 0;
	int maxv = 0;
	for (int vertnum = 1; vertnum < NumVerts; vertnum++)
	{
		if (v[vertnum].y < v[minv].y)
			minv = vertnum;
		else if (v[vertnum].y > v[maxv].y)
			maxv = vertnum;
		minx = std::min(minx, v[vertnum].x);
		maxx = std::max(maxx, v[vertnum].x);
	}

	// determine start/end scanlines
	int32_t miny = round_coordinate(v[minv].y);
	int32_t maxy = round_coordinate(v[maxv].y);

	// clip coordinates
	int32_t minyclip = miny;
	int32_t maxyclip = maxy;
	minyclip = std::max(minyclip, cliprect.min_y);
	maxyclip = std::min(maxyclip, cliprect.max_y + 1);
	if (maxyclip - minyclip <= 0)
		return 0;

	// walk forward to build up the forward edge list
	struct poly_edge
	{
		poly_edge *next;                       // next edge in sequence
		int index;                             // index of this edge
		vertex_t const *v1;                    // pointer to first vertex
		vertex_t const *v2;                    // pointer to second vertex
		BaseType dxdy;                         // dx/dy along the edge
		std::array<BaseType, MaxParams> dpdy;  // per-parameter dp/dy values
	};
	poly_edge fedgelist[NumVerts - 1];
	poly_edge *edgeptr = &fedgelist[0];
	for (int curv = minv; curv != maxv; curv = (curv == NumVerts - 1) ? 0 : (curv + 1))
	{
		// set the two vertices
		edgeptr->v1 = &v[curv];
		edgeptr->v2 = &v[(curv == NumVerts - 1) ? 0 : (curv + 1)];

		// if horizontal, skip altogether
		if (edgeptr->v1->y == edgeptr->v2->y)
			continue;

		// need dx/dy always, and parameter deltas as necessary
		BaseType ooy = BaseType(1.0) / (edgeptr->v2->y - edgeptr->v1->y);
		edgeptr->dxdy = (edgeptr->v2->x - edgeptr->v1->x) * ooy;
		for (int paramnum = 0; paramnum < ParamCount; paramnum++)
			edgeptr->dpdy[paramnum] = (edgeptr->v2->p[paramnum] - edgeptr->v1->p[paramnum]) * ooy;
		++edgeptr;
	}

	// walk backward to build up the backward edge list
	poly_edge bedgelist[NumVerts - 1];
	edgeptr = &bedgelist[0];
	for (int curv = minv; curv != maxv; curv = (curv == 0) ? (NumVerts - 1) : (curv - 1))
	{
		// set the two vertices
		edgeptr->v1 = &v[curv];
		edgeptr->v2 = &v[(curv == 0) ? (NumVerts - 1) : (curv - 1)];

		// if horizontal, skip altogether
		if (edgeptr->v1->y == edgeptr->v2->y)
			continue;

		// need dx/dy always, and parameter deltas as necessary
		BaseType ooy = BaseType(1.0) / (edgeptr->v2->y - edgeptr->v1->y);
		edgeptr->dxdy = (edgeptr->v2->x - edgeptr->v1->x) * ooy;
		for (int paramnum = 0; paramnum < ParamCount; paramnum++)
			edgeptr->dpdy[paramnum] = (edgeptr->v2->p[paramnum] - edgeptr->v1->p[paramnum]) * ooy;
		++edgeptr;
	}

	// determine which list is left/right:
	// if the first vertex is shared, compare the slopes
	// if the first vertex is not shared, compare the X coordinates
	poly_edge const *ledge, *redge;
	if ((fedgelist[0].v1 == bedgelist[0].v1 && fedgelist[0].dxdy < bedgelist[0].dxdy) ||
		(fedgelist[0].v1 != bedgelist[0].v1 && fedgelist[0].v1->x < bedgelist[0].v1->x))
	{
		ledge = fedgelist;
		redge = bedgelist;
	}
	else
	{
		ledge = bedgelist;
		redge = fedgelist;
	}

	// compute the X extents for each scanline
	int32_t pixels = 0;
	extent_t extent;
	for (int32_t curscan = minyclip; curscan < maxyclip; curscan++)
	{
		// compute the ending X based on which part of the triangle we're in
		BaseType fully = BaseType(curscan) + BaseType(0.5);
		while (fully > ledge->v2->y && fully < v[maxv].y)
			++ledge;
		while (fully > redge->v2->y && fully < v[maxv].y)
			++redge;
		BaseType startx = ledge->v1->x + (fully - ledge->v1->y) * ledge->dxdy;
		BaseType stopx = redge->v1->x + (fully - redge->v1->y) * redge->dxdy;

		// clamp to full pixels
		int32_t istartx = round_coordinate(startx);
		int32_t istopx = round_coordinate(stopx);

		// force start < stop
		if (istartx > istopx)
			std::swap(istartx, istopx);

		// apply left/right clipping BEFORE calculating parameter start
		istartx = std::max(istartx, cliprect.min_x);
		istopx = std::min(istopx, cliprect.max_x + 1);

		if (ParamCount > 0)
		{
			BaseType ldy = fully - ledge->v1->y;
			BaseType rdy = fully - redge->v1->y;
			BaseType oox = BaseType(1.0) / (stopx - startx);

			// iterate over parameters
			for (int paramnum = 0; paramnum < ParamCount; paramnum++)
			{
				BaseType lparam = ledge->v1->p[paramnum] + ldy * ledge->dpdy[paramnum];
				BaseType rparam = redge->v1->p[paramnum] + rdy * redge->dpdy[paramnum];

				BaseType dpdx = (rparam - lparam) * oox;

				extent.param[paramnum].start = lparam + (BaseType(istartx) + BaseType(0.5) - startx) * dpdx;
				extent.param[paramnum].dpdx = dpdx;
			}
		}

		// set the extent and update the total pixel count
		if (istartx >= istopx)
			istartx = istopx = 0;
		extent.startx = istartx;
		extent.stopx = istopx;
		pixels += istopx - istartx;
		callback(curscan, extent, obj, 0);
	}
	return pixels;
}
