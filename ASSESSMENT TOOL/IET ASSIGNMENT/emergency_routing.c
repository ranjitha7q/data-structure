#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define MAX_NODES 5000
#define MAX_EDGES 10000
#define HASH_SIZE 10007
#define INF INT_MAX

typedef struct Edge {
    int to;
    int distance;
    int traffic;              /* 1=Low, 2=Moderate, 3=Heavy, 4=Severe */
    struct Edge *next;
} Edge;

typedef struct {
    int node;
    int distance;
} HeapNode;

typedef struct {
    HeapNode *data;
    int size;
    int capacity;
} MinHeap;

typedef struct {
    int key;
    int value;
    int used;
} HashEntry;

typedef struct {
    HashEntry table[HASH_SIZE];
} HashMap;

typedef struct {
    unsigned char visited[MAX_NODES];
} HashSet;

typedef struct {
    int vertices;
    Edge **adj;
    HashMap trafficMap;
    HashSet visited;
} Graph;

static int hash(int key) {
    return (key % HASH_SIZE + HASH_SIZE) % HASH_SIZE;
}

static void hashmap_init(HashMap *map) {
    memset(map, 0, sizeof(*map));
}

static void hashmap_put(HashMap *map, int key, int value) {
    int i = hash(key);
    while (map->table[i].used && map->table[i].key != key)
        i = (i + 1) % HASH_SIZE;
    map->table[i].key = key;
    map->table[i].value = value;
    map->table[i].used = 1;
}

static int hashmap_get(HashMap *map, int key, int *value) {
    int i = hash(key), start = i;
    while (map->table[i].used) {
        if (map->table[i].key == key) {
            *value = map->table[i].value;
            return 1;
        }
        i = (i + 1) % HASH_SIZE;
        if (i == start) break;
    }
    return 0;
}

static void heap_init(MinHeap *h, int capacity) {
    h->data = malloc(sizeof(HeapNode) * capacity);
    h->size = 0;
    h->capacity = capacity;
}

static void heap_push(MinHeap *h, int node, int distance) {
    if (h->size >= h->capacity) return;
    int i = h->size++;
    h->data[i] = (HeapNode){node, distance};
    while (i > 0) {
        int p = (i - 1) / 2;
        if (h->data[p].distance <= h->data[i].distance) break;
        HeapNode t = h->data[p]; h->data[p] = h->data[i]; h->data[i] = t;
        i = p;
    }
}

static HeapNode heap_pop(MinHeap *h) {
    HeapNode root = h->data[0];
    h->data[0] = h->data[--h->size];
    int i = 0;
    while (1) {
        int l = 2*i+1, r = 2*i+2, s = i;
        if (l < h->size && h->data[l].distance < h->data[s].distance) s = l;
        if (r < h->size && h->data[r].distance < h->data[s].distance) s = r;
        if (s == i) break;
        HeapNode t = h->data[i]; h->data[i] = h->data[s]; h->data[s] = t;
        i = s;
    }
    return root;
}

static int traffic_multiplier(int level) {
    return level; /* demonstration weight adjustment */
}

Graph *graph_create(int vertices) {
    Graph *g = calloc(1, sizeof(Graph));
    g->vertices = vertices;
    g->adj = calloc(vertices, sizeof(Edge *));
    hashmap_init(&g->trafficMap);
    return g;
}

void add_road(Graph *g, int u, int v, int distance, int traffic) {
    Edge *e = malloc(sizeof(Edge));
    *e = (Edge){v, distance, traffic, g->adj[u]};
    g->adj[u] = e;
    hashmap_put(&g->trafficMap, u * MAX_NODES + v, traffic);
}

void update_traffic(Graph *g, int u, int v, int newTraffic) {
    for (Edge *e = g->adj[u]; e; e = e->next) {
        if (e->to == v) {
            e->traffic = newTraffic;
            hashmap_put(&g->trafficMap, u * MAX_NODES + v, newTraffic);
            return;
        }
    }
}

int dijkstra(Graph *g, int source, int destination, int *previous) {
    int *dist = malloc(sizeof(int) * g->vertices);
    for (int i = 0; i < g->vertices; ++i) {
        dist[i] = INF;
        previous[i] = -1;
        g->visited.visited[i] = 0;
    }

    MinHeap heap;
    heap_init(&heap, g->vertices * 4);
    dist[source] = 0;
    heap_push(&heap, source, 0);

    while (heap.size) {
        HeapNode cur = heap_pop(&heap);
        int u = cur.node;
        if (g->visited.visited[u]) continue;
        g->visited.visited[u] = 1;
        if (u == destination) break;

        for (Edge *e = g->adj[u]; e; e = e->next) {
            if (g->visited.visited[e->to]) continue;
            int weight = e->distance * traffic_multiplier(e->traffic);
            if (dist[u] != INF && dist[u] + weight < dist[e->to]) {
                dist[e->to] = dist[u] + weight;
                previous[e->to] = u;
                heap_push(&heap, e->to, dist[e->to]);
            }
        }
    }

    int answer = dist[destination];
    free(heap.data);
    free(dist);
    return answer;
}

void print_path(int previous[], int source, int destination) {
    int path[MAX_NODES], n = 0;
    for (int v = destination; v != -1; v = previous[v])
        path[n++] = v;

    for (int i = n - 1; i >= 0; --i) {
        printf("%d%s", path[i], i ? " -> " : "\n");
    }
}

void graph_free(Graph *g) {
    for (int i = 0; i < g->vertices; ++i) {
        Edge *e = g->adj[i];
        while (e) { Edge *next = e->next; free(e); e = next; }
    }
    free(g->adj);
    free(g);
}

int main(void) {
    Graph *g = graph_create(6);

    /* Sample city network */
    add_road(g, 0, 1, 8, 1);
    add_road(g, 0, 2, 4, 1);
    add_road(g, 1, 2, 2, 2);
    add_road(g, 1, 3, 7, 1);
    add_road(g, 2, 3, 5, 1);
    add_road(g, 2, 4, 10, 2);
    add_road(g, 3, 4, 3, 1);
    add_road(g, 3, 5, 6, 1);
    add_road(g, 4, 5, 2, 1);

    int previous[MAX_NODES];
    int source = 0, destination = 5;

    int cost = dijkstra(g, source, destination, previous);

    printf("Smart-City Emergency Vehicle Routing System\n");
    printf("Shortest available route: ");
    print_path(previous, source, destination);
    printf("Traffic-adjusted cost: %d\n", cost);

    /* Dynamic traffic update example */
    update_traffic(g, 2, 3, 4);
    cost = dijkstra(g, source, destination, previous);
    printf("\nAfter traffic update on road 2 -> 3:\n");
    printf("Updated route: ");
    print_path(previous, source, destination);
    printf("Updated traffic-adjusted cost: %d\n", cost);

    graph_free(g);
    return 0;
}