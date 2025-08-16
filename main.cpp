#include <iostream>
#include <vector>
#include <fstream>
#include <numeric>
#include <random>
#include <chrono>

// --- Perlin Noise Class Implementation (reused from previous success) ---
class PerlinNoise {
private:
    std::vector<int> p;

    // A helper function to find the maximum value in a vector.
    template <typename T>
    T findMax(const std::vector<T>& v) {
        if (v.empty()) {
            return T();
        }
        T max_val = v[0];
        for (const T& val : v) {
            if (val > max_val) {
                max_val = val;
            }
        }
        return max_val;
    }

    // A helper function to find the minimum value in a vector.
    template <typename T>
    T findMin(const std::vector<T>& v) {
        if (v.empty()) {
            return T();
        }
        T min_val = v[0];
        for (const T& val : v) {
            if (val < min_val) {
                min_val = val;
            }
        }
        return min_val;
    }

    // The fade function as defined by Perlin.
    double fade(double t) { return t * t * t * (t * (t * 6 - 15) + 10); }

    // Linear interpolation.
    double lerp(double t, double a, double b) { return a + t * (b - a); }

    // Gradient function to get the dot product of a randomly selected gradient vector
    // and the distance vector.
    double grad(int hash, double x, double y, double z) {
        int h = hash & 15;
        double u = h < 8 ? x : y;
        double v = h < 4 ? y : (h == 12 || h == 14) ? x : z;
        return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
    }

public:
    PerlinNoise(unsigned int seed) {
        p.resize(256);
        std::iota(p.begin(), p.end(), 0);
        std::default_random_engine engine(seed);
        std::shuffle(p.begin(), p.end(), engine);
        p.insert(p.end(), p.begin(), p.end());
    }

    double noise(double x, double y, double z) {
        int X = static_cast<int>(floor(x)) & 255;
        int Y = static_cast<int>(floor(y)) & 255;
        int Z = static_cast<int>(floor(z)) & 255;

        x -= floor(x);
        y -= floor(y);
        z -= floor(z);

        double u = fade(x);
        double v = fade(y);
        double w = fade(z);

        int A = p[X] + Y;
        int AA = p[A] + Z;
        int AB = p[A + 1] + Z;
        int B = p[X + 1] + Y;
        int BA = p[B] + Z;
        int BB = p[B + 1] + Z;

        return lerp(w, lerp(v, lerp(u, grad(p[AA], x, y, z),
                                     grad(p[BA], x - 1, y, z)),
                             lerp(u, grad(p[AB], x, y - 1, z),
                                     grad(p[BB], x - 1, y - 1, z))),
                    lerp(v, lerp(u, grad(p[AA + 1], x, y, z - 1),
                                     grad(p[BA + 1], x - 1, y, z - 1)),
                             lerp(u, grad(p[AB + 1], x, y - 1, z - 1),
                                     grad(p[BB + 1], x - 1, y - 1, z - 1))));
    }
};

// --- Quadtree Class Implementation ---
// A Point structure to store coordinates and height.
struct Point {
    int x, y;
    int height;
};

// A Rectangle class to define the bounds of a quadtree node.
class Rectangle {
public:
    int x, y, w, h;
    Rectangle(int _x, int _y, int _w, int _h) : x(_x), y(_y), w(_w), h(_h) {}

    bool contains(const Point& point) const {
        return (point.x >= x && point.x < x + w &&
                point.y >= y && point.y < y + h);
    }
};

// The Quadtree class.
class Quadtree {
private:
    Rectangle boundary;
    std::vector<Point> points;
    int capacity;
    bool subdivided;
    int maxDepth;
    std::vector<std::vector<int>>& heightmap;

    Quadtree* northwest;
    Quadtree* northeast;
    Quadtree* southwest;
    Quadtree* southeast;

    // Checks for height variation within the boundary
    bool needsSubdivision() {
        int min_h = 256, max_h = -1;
        for (int i = boundary.y; i < boundary.y + boundary.h; ++i) {
            for (int j = boundary.x; j < boundary.x + boundary.w; ++j) {
                int h = heightmap[i][j];
                if (h > max_h) max_h = h;
                if (h < min_h) min_h = h;
            }
        }
        // Subdivide if the height range is greater than a threshold,
        // unless it's a very small node (2x2)
        return (max_h - min_h) > 50 && (boundary.w > 2 && boundary.h > 2);
    }

    void subdivide() {
        int x = boundary.x;
        int y = boundary.y;
        int w = boundary.w / 2;
        int h = boundary.h / 2;

        northwest = new Quadtree(Rectangle(x, y, w, h), capacity, heightmap, maxDepth);
        northeast = new Quadtree(Rectangle(x + w, y, w, h), capacity, heightmap, maxDepth);
        southwest = new Quadtree(Rectangle(x, y + h, w, h), capacity, heightmap, maxDepth);
        southeast = new Quadtree(Rectangle(x + w, y + h, w, h), capacity, heightmap, maxDepth);

        subdivided = true;
    }

public:
    Quadtree(Rectangle _boundary, int _capacity, std::vector<std::vector<int>>& _heightmap, int _maxDepth) :
        boundary(_boundary), capacity(_capacity), heightmap(_heightmap), maxDepth(_maxDepth), subdivided(false),
        northwest(nullptr), northeast(nullptr), southwest(nullptr), southeast(nullptr) {}

    ~Quadtree() {
        delete northwest;
        delete northeast;
        delete southwest;
        delete southeast;
    }

    // Insert a single point. This function is recursive.
    bool insert(const Point& point) {
        if (!boundary.contains(point)) {
            return false;
        }

        if (subdivided) {
            if (northwest->insert(point)) return true;
            if (northeast->insert(point)) return true;
            if (southwest->insert(point)) return true;
            if (southeast->insert(point)) return true;
        } else {
            // Check if this node is a leaf node and needs to be subdivided based on height variance
            if (needsSubdivision()) {
                subdivide();
                return insert(point);
            }
            points.push_back(point);
            return true;
        }

        return false;
    }

    // Recursive function to draw the quadtree on the heightmap.
    void draw(std::vector<std::vector<unsigned char>>& image_data) const {
        // Draw the boundaries of the current node
        for (int i = boundary.x; i < boundary.x + boundary.w; ++i) {
            image_data[boundary.y][i] = 150; // A darker gray for the line
            image_data[boundary.y + boundary.h - 1][i] = 150;
        }
        for (int i = boundary.y; i < boundary.y + boundary.h; ++i) {
            image_data[i][boundary.x] = 150;
            image_data[i][boundary.x + boundary.w - 1] = 150;
        }

        // Recursively draw sub-nodes
        if (subdivided) {
            northwest->draw(image_data);
            northeast->draw(image_data);
            southwest->draw(image_data);
            southeast->draw(image_data);
        }
    }
};

// --- Main Program ---
int main() {
    const int WIDTH = 800;
    const int HEIGHT = 800;
    const double SCALE = 100.0;
    const int OCTAVES = 4;
    const double PERSISTENCE = 0.5;

    // Use current time to seed the noise generator for different results each time
    unsigned seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    PerlinNoise perlin(seed);

    // Store heightmap data
    std::vector<std::vector<int>> heightmap(HEIGHT, std::vector<int>(WIDTH));
    std::vector<unsigned char> image_data(WIDTH * HEIGHT * 3);

    // Generate Perlin noise heightmap
    for (int y = 0; y < HEIGHT; ++y) {
        for (int x = 0; x < WIDTH; ++x) {
            double totalNoise = 0.0;
            double amplitude = 1.0;
            double frequency = 1.0;
            double maxValue = 0.0;

            for (int i = 0; i < OCTAVES; ++i) {
                totalNoise += perlin.noise(x / SCALE * frequency, y / SCALE * frequency, 0) * amplitude;
                maxValue += amplitude;
                amplitude *= PERSISTENCE;
                frequency *= 2.0;
            }

            double normalizedValue = (totalNoise / maxValue + 1) / 2;
            int value = static_cast<int>(normalizedValue * 255.0);
            heightmap[y][x] = value;
        }
    }

    // Build the quadtree
    Rectangle boundary(0, 0, WIDTH, HEIGHT);
    Quadtree quadtree(boundary, 4, heightmap, 10);

    // Insert a single point to trigger the subdivision process
    quadtree.insert({0, 0, heightmap[0][0]});

    // Write the PPM file
    std::ofstream outfile("output_with_quadtree.ppm", std::ios::out | std::ios::binary);
    if (!outfile.is_open()) {
        std::cerr << "Error: Could not open output file." << std::endl;
        return 1;
    }

    outfile << "P6\n" << WIDTH << " " << HEIGHT << "\n255\n";

    // Draw the image data with the quadtree boundaries
    std::vector<std::vector<unsigned char>> final_image(HEIGHT, std::vector<unsigned char>(WIDTH));
    for (int y = 0; y < HEIGHT; ++y) {
        for (int x = 0; x < WIDTH; ++x) {
            final_image[y][x] = heightmap[y][x];
        }
    }
    
    // Draw the quadtree boundaries on the image
    quadtree.draw(final_image);

    // Write the final pixel data to the file
    for (int y = 0; y < HEIGHT; ++y) {
        for (int x = 0; x < WIDTH; ++x) {
            unsigned char color = final_image[y][x];
            outfile.put(color);
            outfile.put(color);
            outfile.put(color);
        }
    }
    
    outfile.close();
    std::cout << "output_with_quadtree.ppm was successfully created." << std::endl;

    return 0;
}
