#include "pch.h"
#include "SDF.h"

#include "Logging.h"

// SDF algorithm: 8SSEDT
// Translation of implementation found at www.codersnotes.com/notes/signed-distance-fields/
// for variable w/h, and slightly more "classed"
// fetched 12/06/2017 DD/MM/YYYY
// supposedly O(n) according to source, should be quick for our uses

// comments added for myself

#define INF 10000

// Internal structures
struct Point {
	int dx, dy;

	int DistSqr() {
		return dx*dx + dy*dy;
	}
};

struct Grid {
public:
	Point* point; // faster on debug than a vector. sorry lads
	int w, h;
	int s; 

	Grid() {
		point = NULL;
	}
	~Grid() {
		delete[] point;
	}

	void Resize(int w, int h) {
		this->w = w;
		this->h = h;
		point = new Point[w * h];
		s = w * h;
	}

	Point Get(int x, int y) {
		int idx = y * w + x;
		if ( y >= h || y < 0 || x >= w || x < 0 )
			return { INF, INF };

		return point[idx];
	}

	void Put(int x, int y, Point v) {
		point[y * w + x] = v;
	}

	int DistSqr(int x, int y) {
		return Get(x, y).DistSqr();
	}

	void Compare(Point &p, int x, int y, int ox, int oy) {
		Point other = Get(x + ox, y + oy);
		other.dx += ox;
		other.dy += oy;

		// minimize distance
		if (other.DistSqr() < p.DistSqr())
		{
			p = other;
		}
	}
};

void GenSDF(Grid &g)
{
	// no idea what these passes do 
	// but hey, a working implementation says it's like this
	
	for (int y = 0; y < g.h; y++) {
		for (int x = 0; x < g.w; x++) {
			Point p = g.Get(x, y);
			g.Compare(p, x, y, -1, 0 );
			g.Compare(p, x, y,  0, -1);
			g.Compare(p, x, y, -1, -1);
			g.Compare(p, x, y,  1, -1 );
			g.Put(x, y, p);
		}

		// backwards from height
		for (int x = g.w - 1; x >= 0; x--) {
			Point p = g.Get(x, y);
			g.Compare(p, x, y, 1, 0);
			g.Put(x, y, p);
		}
	}

	for (int y = g.h - 1; y >= 0; y--)
	{
		for (int x = g.w - 1; x >= 0; x--)
		{
			Point p = g.Get(x, y);
			g.Compare(p, x, y,  1, 0);
			g.Compare(p, x, y,  0, 1);
			g.Compare(p, x, y, -1, 1);
			g.Compare(p, x, y,  1, 1);
			g.Put(x, y, p);
		}

		for (int x = 0; x < g.w; x++) {
			Point p = g.Get(x, y);
			g.Compare(p, x, y, -1, 0);
			g.Put(x, y, p);
		}
	}
}

void ConvertToSDF(unsigned char* out, unsigned char* tex, int w, int h) {
	// Generate initial SDF
	Grid g1, g2;
	g1.Resize(w, h);
	g2.Resize(w, h);


	// build grids
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			int idx = y * w + x;
			if (tex[idx] < 128) {
				g1.Put(x, y, { 0, 0 });
				g2.Put(x, y, { INF, INF });
			}
			else {
				g2.Put(x, y, { 0, 0 });
				g1.Put(x, y, { INF, INF });
			}
		}
	}

	// "propagate"
	GenSDF(g1);
	GenSDF(g2);

	int max = -INF;
	int min = INF;

	std::vector<int> preout(w * h);
	// copy to float GL_ALPHA texture
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			auto d1 = sqrt(g1.DistSqr(x, y));
			auto d2 = sqrt(g2.DistSqr(x, y));
			int dist = round(d1 - d2);

			// dist = dist * 3 + 128;
			
			if (dist > max) max = dist;
			if (dist < min) min = dist;

			preout[y * w + x] = dist;
		}
	}

	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			auto v = preout[y * w + x];
			//float o = round(float(v - min) / float(max - min) * 255.0);

			if (v < 0) {
				float o = round(v * -128.0 / min + 128);
				int d = Clamp((int)o, 0, 255);
				out[y * w + x] = d;
			}
			else {
				float o = round(v * 127 / max + 128);
				int d = Clamp((int)o, 0, 255);
				out[y * w + x] = d;
			}
		}
	}
	
	// Log::Printf("Max/Min SDF value: %d/%d\n", max, min);
}