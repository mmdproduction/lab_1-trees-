#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <stdexcept>
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

public:
    enum class PartKind {
        Root,
        Child
    };

    struct Part {
        PartKind kind;
        std::size_t child_index;

        static Part root() {
            return {PartKind::Root, 0};
        }

        static Part child(std::size_t index) {
            return {PartKind::Child, index};
        }
    };

    using OrderPolicy = std::vector<Part>;

    static OrderPolicy klp() {
        return {Part::root(), Part::child(0), Part::child(1)};
    }

    static OrderPolicy kpl() {
        return {Part::root(), Part::child(1), Part::child(0)};
    }

    static OrderPolicy lkp() {
        return {Part::child(0), Part::root(), Part::child(1)};
    }

    static OrderPolicy lpk() {
        return {Part::child(0), Part::child(1), Part::root()};
    }

    static OrderPolicy pkl() {
        return {Part::child(1), Part::root(), Part::child(0)};
    }

    static OrderPolicy plk() {
        return {Part::child(1), Part::child(0), Part::root()};
    }


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

    static void validateOrder(const OrderPolicy& policy) {
        if (policy.size() != 3) {
            throw std::invalid_argument(
                "Traversal policy must contain 3 parts"
            );
        }

        bool rootSeen = false;
        bool childSeen[2] = {false, false};

        for (const Part& part : policy) {
            switch (part.kind) {
            case PartKind::Root:
                if (rootSeen) {
                    throw std::invalid_argument("Repeated root");
                }

                rootSeen = true;
                break;

            case PartKind::Child:
                if (part.child_index >= 2) {
                    throw std::invalid_argument("Invalid child index");
                }

                if (childSeen[part.child_index]) {
                    throw std::invalid_argument("Repeated child");
                }

                childSeen[part.child_index] = true;
                break;

            default:
                throw std::invalid_argument("Invalid part kind");
            }
        }
    }

        template<class Func>
    static void traverseNode(
        const std::unique_ptr<Node>& node,
        const OrderPolicy& policy,
        Func& action
    ) {
        if (!node) {
            return;
        }

        for (const Part& part : policy) {
            switch (part.kind) {
            case PartKind::Root:
                action(std::as_const(node->value));
                break;

            case PartKind::Child: {
                const auto& child = (part.child_index == 0)
                    ? node->left
                    : node->right;

                traverseNode(child, policy, action);
                break;
            }
            }
        }
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

    template<class Func>
    void traverse(const OrderPolicy& policy, Func&& action) const {
        validateOrder(policy);
        traverseNode(root, policy, action);
    }
};

