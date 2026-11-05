#pragma once

#include <cstddef>
#include <functional>
#include <iomanip>
#include <limits>
#include <memory>
#include <optional>
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
    std::unique_ptr<Node> root_;
    std::size_t count_ = 0;
    Compare comp_;

    static std::unique_ptr<Node> clone(
        const std::unique_ptr<Node>& node
    ) {
        if (!node) {
            return nullptr;
        }

        auto copy = std::make_unique<Node>(node->value);

        copy->left = clone(node->left);
        copy->right = clone(node->right);
        copy->height = node->height;

        return copy;
    }

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
            ++count_;
            return true;
        }

        bool inserted;

        if (comp_(value, node->value)) {
            inserted = insertNode(node->left, value);
        } else if (comp_(node->value, value)) {
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

        if (comp_(value, node->value)) {
            erased = eraseNode(node->left, value);
        } else if (comp_(node->value, value)) {
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

            --count_;
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

    static std::size_t countNodes(
        const std::unique_ptr<Node>& node
    ) {
        if (!node) {
            return 0;
        }

        return 1 + countNodes(node->left)
                + countNodes(node->right);
    }

    bool sameSubtree(
        const std::unique_ptr<Node>& first,
        const std::unique_ptr<Node>& second
    ) const {
        if (!first || !second) {
            return !first && !second;
        }

        if (comp_(first->value, second->value) ||
            comp_(second->value, first->value)) {
            return false;
        }

        return sameSubtree(first->left, second->left)
            && sameSubtree(first->right, second->right);
    }

    static void serializeNode(
        const std::unique_ptr<Node>& node,
        std::ostream& output
    ) {
        if (!node) {
            return;
        }

        std::ostringstream valueStream;

        if constexpr (std::is_floating_point_v<T>) {
            valueStream << std::setprecision(
                std::numeric_limits<T>::max_digits10
            );
        }

        valueStream << node->value;

        if (!valueStream) {
            throw std::runtime_error("Cannot serialize value");
        }

        output << '{';

        for (char ch : valueStream.str()) {
            if (ch == '}' || ch == '\\') {
                output << '\\';
            }

            output << ch;
        }

        output << '}';

        if (!node->left && !node->right) {
            return;
        }


        output << '(';
        serializeNode(node->left, output);
        output << ')';

        output << '[';
        serializeNode(node->right, output);
        output << ']';
    }


    static std::string readValueText(std::istream& input) {
        std::string text;
        char ch;

        while (input.get(ch)) {
            if (ch == '}') {
                return text;
            }

            if (ch == '\\') {
                if (!input.get(ch) || (ch != '}' && ch != '\\')) {
                    throw std::invalid_argument("Invalid escape sequence");
                }
            }

            text += ch;
        }

        throw std::invalid_argument("Unterminated value");
    }

    static void expectChar(std::istream& input, char expected) {
        char actual;

        if (!input.get(actual) || actual != expected) {
            throw std::invalid_argument("Invalid tree format");
        }
    }

    static T parseValue(const std::string& text) {
        if constexpr (std::is_same_v<T, std::string>) {
            return text;
        } else {
            T value{};
            std::istringstream input(text);

            if (!(input >> value)) {
                throw std::invalid_argument("Invalid value");
            }

            input >> std::ws;

            if (!input.eof()) {
                throw std::invalid_argument("Extra characters in value");
            }

            return value;
        }
    }

    static std::unique_ptr<Node> deserializeNode(
        std::istream& input,
        std::size_t depth = 0
    ) {

        if (depth > 128) {
            throw std::invalid_argument("Tree is too deep");
        }

        expectChar(input, '{');

        const std::string text = readValueText(input);
        auto node = std::make_unique<Node>(parseValue(text));

        if (input.peek() != '(') {
            return node;
        }

        expectChar(input, '(');

        if (input.peek() != ')') {
            node->left = deserializeNode(input, depth + 1);
        }

        expectChar(input, ')');
        expectChar(input, '[');

        if (input.peek() != ']') {
            node->right = deserializeNode(input, depth + 1);
        }

        expectChar(input, ']');

        updateHeight(node.get());

        const int balance = getBalance(node.get());

        if (balance < -1 || balance > 1) {
            throw std::invalid_argument("Tree is not AVL balanced");
        }

        return node;
    }

    std::size_t validateTree(
        const std::unique_ptr<Node>& node,
        const std::optional<std::reference_wrapper<const T>>& lower,
        const std::optional<std::reference_wrapper<const T>>& upper
    ) const {
        if (!node) {
            return 0;
        }

        if (lower && !comp_(lower->get(), node->value)) {
            throw std::invalid_argument("Invalid BST order");
        }

        if (upper && !comp_(node->value, upper->get())) {
            throw std::invalid_argument("Invalid BST order");
        }

        const auto bound =
            std::optional<std::reference_wrapper<const T>>{
                std::cref(node->value)
            };

        return 1
            + validateTree(node->left, lower, bound)
            + validateTree(node->right, bound, upper);
    }
    
public:

    AVLTree() = default;

    AVLTree(const AVLTree& other)
        : root_(clone(other.root_)),
        count_(other.count_),
        comp_(other.comp_) {}

    void swap(AVLTree& other) noexcept {
        static_assert(
            std::is_nothrow_swappable_v<Compare>,
            "Compare must support noexcept swap"
        );

        using std::swap;

        swap(comp_, other.comp_);
        root_.swap(other.root_);
        swap(count_, other.count_);
    }

    AVLTree& operator=(const AVLTree& other) {
        if (this != &other) {
            AVLTree copy(other);
            swap(copy);
        }

        return *this;
    }

    AVLTree(AVLTree&& other)
        : comp_(other.comp_) {
        root_ = std::move(other.root_);
        count_ = std::exchange(other.count_, 0);
    }

    AVLTree& operator=(AVLTree&& other) {
        if (this != &other) {
            AVLTree moved(std::move(other));
            swap(moved);
        }

        return *this;
    }

    void clear() noexcept {
        root_.reset();
        count_ = 0;
    }
    
    bool insert(const T& value) {
        return insertNode(root_, value);
    }

    bool erase(const T& value){
        return eraseNode(root_, value);
    }

    std::size_t size() const noexcept {
        return count_;
    }

    bool empty() const noexcept {
        return count_ == 0;
    }

    bool contains(const T& value) const
    {
        const Node* x = root_.get();
        while(x)
        {
            if(comp_(value, x->value)){
                x = x->left.get();
            } else if(comp_(x->value, value)){
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
        traverseNode(root_, policy, action);
    }

    template<class Func>
    AVLTree map(Func action) const {
        AVLTree result;
        result.comp_ = comp_;

        traverse(lkp(), [&](const T& value) {
            result.insert(action(value));
        });

        return result;
    }

    template<class Predicate>
    AVLTree where(Predicate predicate) const {
        AVLTree result;
        result.comp_ = comp_;

        traverse(lkp(), [&](const T& value) {
            if (predicate(value)) {
                result.insert(value);
            }
        });

        return result;
    }

    template<class Result, class Func>
    Result reduce(Func action, Result initial) const {
        traverse(lkp(), [&](const T& value) {
            initial = action(initial, value);
        });

        return initial;
    }

    AVLTree getSubtree(const T& value) const {
        auto current = std::cref(root_);

        while (current.get()) {
            const auto& node = current.get();

            if (comp_(value, node->value)) {
                current = std::cref(node->left);
            } else if (comp_(node->value, value)) {
                current = std::cref(node->right);
            } else {
                AVLTree result;
                result.comp_ = comp_;
                result.root_ = clone(node);
                result.count_ = countNodes(result.root_);

                return result;
            }
        }

        throw std::out_of_range("Subtree root not found");
    }

    bool containsSubtree(const AVLTree& subtree) const {
        if (!subtree.root_) {
            return true;
        }

        auto current = std::cref(root_);
        const T& value = subtree.root_->value;

        while (current.get()) {
            const auto& node = current.get();

            if (comp_(value, node->value)) {
                current = std::cref(node->left);
            } else if (comp_(node->value, value)) {
                current = std::cref(node->right);
            } else {
                return sameSubtree(node, subtree.root_);
            }
        }

        return false;
    }

    std::string serialize() const {
        std::ostringstream output;
        serializeNode(root_, output);
        return output.str();
    }

    void deserialize(const std::string& text) {
        std::unique_ptr<Node> parsed;
        std::size_t parsedCount = 0;

        if (!text.empty()) {
            std::istringstream input(text);

            parsed = deserializeNode(input);

            if (input.peek() != std::char_traits<char>::eof()) {
                throw std::invalid_argument("Extra data after tree");
            }

            parsedCount = validateTree(
                parsed,
                std::nullopt,
                std::nullopt
            );
        }

        root_ = std::move(parsed);
        count_ = parsedCount;
    }
};



