#pragma once


class Node
{
public:
    constexpr Node(int v) : val(v), next(nullptr) {}

    constexpr void setNext(Node* p) {
        next = p;
    }

    constexpr Node* getNext() {
        return next;
    }

private:
    int val;
    Node* next; /* a pointer to next node */

};


constexpr void insert(Node* n0, Node* p) {
    Node* n1 = n0->getNext();
    n0->setNext(p);
    p->setNext(n1);
}


constexpr Node* access(Node* beg, int idx) {
    Node* tmp = beg;
    while (idx-- > 0 and tmp != nullptr) {
        tmp = tmp->getNext();
    }
    return tmp;
}

