#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <utility>

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

    static std::unique_ptr<Node> rotateLeft(std::unique_ptr<Node> y){
        auto x = std::move(y->right);

        y->right = std::move(x->left);
        x->left = std::move(y);

        updateHeight(x->left.get());
        updateHeight(x.get());

        return x;
    }

    static std::unique_ptr<Node> rotateRight(std::unique_ptr<Node> y){
         auto x = std::move(y->left);

        y->left = std::move(x->right);
        x->right = std::move(y);

        updateHeight(x->right.get());
        updateHeight(x.get());

        return x;
    }

    static std::unique_ptr<Node> rebalance(std::unique_ptr<Node> node) {
        if (!node) {
            return node;
        }

        updateHeight(node.get());
        const int balance = getBalance(node.get());

        if (balance > 1) {
            if (getBalance(node->left.get()) < 0) {
                node->left = rotateLeft(std::move(node->left));
            }

            return rotateRight(std::move(node));
        }

        if (balance < -1) {
            if (getBalance(node->right.get()) > 0) {
                node->right = rotateRight(std::move(node->right));
            }

            return rotateLeft(std::move(node));
        }

        return node;
    }

    bool insertNode(std::unique_ptr<Node>& node, const T& value) {
        if (!node) {
            node = std::make_unique<Node>(value);
            ++count;
            return true;
        }

        bool inserted;

        if (comp(value, node->value)) {
            inserted = insertNode(node->left, value);
        } else if (comp(node->value, value)) {
            inserted = insertNode(node->right, value);
        } else {
            return false;
        }

        if (inserted) {
            node = rebalance(std::move(node));
        }

        return inserted;
    }

    static std::unique_ptr<Node> extractMin(
        std::unique_ptr<Node>& node
    ) {
        if (!node->left) {
            auto minimum = std::move(node);
            node = std::move(minimum->right);
            return minimum;
        }

        auto minimum = extractMin(node->left);
        node = rebalance(std::move(node));

        return minimum;
    }

    bool eraseNode(std::unique_ptr<Node>& node, const T& value) {
        if (!node) {
            return false;
        }

        bool erased;

        if (comp(value, node->value)) {
            erased = eraseNode(node->left, value);
        } else if (comp(node->value, value)) {
            erased = eraseNode(node->right, value);
        } else {
            auto removed = std::move(node);

            if (!removed->left) {
                node = std::move(removed->right);
            } else if (!removed->right) {
                node = std::move(removed->left);
            } else {
                auto replacement = extractMin(removed->right);

                replacement->left = std::move(removed->left);
                replacement->right = std::move(removed->right);

                node = std::move(replacement);
            }

            --count;
            erased = true;
        }

        if (erased && node) {
            node = rebalance(std::move(node));
        }

        return erased;
    }
    
public:
    bool insert(const T& value) {
        return insertNode(root, value);
    }

    bool erase(const T& value){
        return eraseNode(root, value);
    }

    std::size_t size() const noexcept {
        return count;
    }

    bool empty() const noexcept {
        return count == 0;
    }

    bool contains(const T& value) const
    {
        const Node* x = root.get();
        while(x)
        {
            if(comp(value, x->value)){
                x = x->left.get();
            } else if(comp(x->value, value)){
                x = x->right.get();
            } else{
                return true;
            }
        }
        return false;
    }
};