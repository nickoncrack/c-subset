struct Node {
    int value;
    struct Node *next;
    int (*handler)(int, int);
};

struct Container {
    struct Node *nodes[10];
    int count;
};

int add(int a, int b) {
    return a + b;
}

int process(struct Node *node, int index) {
    int x = node->value;
    int *ptr = &x;
    int arr[5];

    arr[index] = *ptr + 2;

    if (x == 0) {
        x++;
    } else if (x > 10) {
        x -= 2;
    } else if (x <= 5) {
        x *= 2;
    } else {
        x >>= 2;
    }

    while (x != 0) {
        x--;

        if (x == 3) continue;
        if (x < 0) break;
    }

    for (int i = 0; i < 5; i++) {
        arr[i] += x;
        if (arr[i] >= 20) arr[i] = 0;
        else if (arr[i] != 5) arr[i]++;
    }

    return node->handler(arr[index], add(x, *ptr));
}

int main(int argc) {
    struct Node node;
    struct Container container;

    int x = 2;
    int y = 3;
    int *p = &x;
    int **pp = &p;

    node.value = x;
    node.next = 0;
    node.handler = add;

    container.nodes[0] = &node;
    container.count = 1;

    if (container.count > 0) {
        struct Node *current = container.nodes[0];
        if (current->next == 0) {
            current->value += y;
        } else if (current->value <= 10) {
            current->value *= 2;
        } else {
            current->value >>= 2;
        }
    } else {
        x = 0;
    }

    x = (*pp) + (x << 2) | (y >> 1);
    x &= 255;
    x |= 1;
    x ^= 2;

    for (int i = 0; i < 10; i++) {
        container.nodes[i] = 0;
        if (i == 5) break;
        else if (i & 1) continue;
    }

    return (int) sizeof(struct Node) + process(&node, x);
}
