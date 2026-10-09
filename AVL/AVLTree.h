#pragma once

#include <cstddef>
#include <functional>
#include <memory>

template< typename T, class Compare = std::less<T>>
class AVLTree{

    struct Node{
        T value;
        std::unique_ptr<Node> left;
        std::unique_ptr<Node> right;
        int height = 1;

        explicit Node(const T& val): value(val){}
    };

    private:
    std::unique_ptr<Node> root;
    std::size_t count = 0;
    Compare comp;

    static int getHeight(const Node* node) {
        return node ? node->height : 0;
    }

    static void updateHeight(Node* node) {
        const int leftHeight = getHeight(node->left.get());
        const int rightHeight = getHeight(node->right.get());

        node->height =
            1 + (leftHeight > rightHeight ? leftHeight : rightHeight);
    }

    static int getBalance(const Node* node) {
        return getHeight(node->left.get())
            - getHeight(node->right.get());
    }
};