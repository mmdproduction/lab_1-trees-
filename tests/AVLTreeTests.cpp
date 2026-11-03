#include <gtest/gtest.h>
#include "AVLTree.h"

TEST(AVLTreeCopy, CopiesEmptyTree) {
    AVLTree<int> original;

    AVLTree<int> copy(original);

    EXPECT_TRUE(copy.empty());
    EXPECT_EQ(copy.size(), 0u);
}

TEST(AVLTreeCopy, CopiesAllElements) {
    AVLTree<int> original;

    for (int value : {20, 10, 30, 5, 15}) {
        original.insert(value);
    }

    AVLTree<int> copy(original);

    EXPECT_EQ(copy.size(), original.size());

    for (int value : {20, 10, 30, 5, 15}) {
        EXPECT_TRUE(copy.contains(value));
    }

    EXPECT_FALSE(copy.contains(100));
}

TEST(AVLTreeCopy, ChangesToCopyDoNotAffectOriginal) {
    AVLTree<int> original;
    original.insert(20);
    original.insert(10);
    original.insert(30);

    AVLTree<int> copy(original);

    EXPECT_TRUE(copy.erase(10));
    EXPECT_TRUE(copy.insert(40));

    EXPECT_TRUE(original.contains(10));
    EXPECT_FALSE(original.contains(40));
    EXPECT_EQ(original.size(), 3u);

    EXPECT_FALSE(copy.contains(10));
    EXPECT_TRUE(copy.contains(40));
    EXPECT_EQ(copy.size(), 3u);
}

TEST(AVLTreeCopy, ChangesToOriginalDoNotAffectCopy) {
    AVLTree<int> original;
    original.insert(20);
    original.insert(10);

    AVLTree<int> copy(original);

    original.erase(20);
    original.insert(30);

    EXPECT_TRUE(copy.contains(20));
    EXPECT_FALSE(copy.contains(30));
    EXPECT_EQ(copy.size(), 2u);
}

TEST(AVLTreeCopy, CopySurvivesOriginalDestruction) {
    AVLTree<int> copy = [] {
        AVLTree<int> original;
        original.insert(20);
        original.insert(10);
        original.insert(30);

        return AVLTree<int>(original);
    }();

    EXPECT_EQ(copy.size(), 3u);
    EXPECT_TRUE(copy.contains(20));
    EXPECT_TRUE(copy.contains(10));
    EXPECT_TRUE(copy.contains(30));

    EXPECT_TRUE(copy.erase(20));
    EXPECT_EQ(copy.size(), 2u);
}

TEST(AVLTreeMove, ConstructorTransfersElements) {
    AVLTree<int> original;
    original.insert(20);
    original.insert(10);

    AVLTree<int> moved(std::move(original));

    EXPECT_EQ(moved.size(), 2u);
    EXPECT_TRUE(moved.contains(20));
    EXPECT_TRUE(moved.contains(10));

    EXPECT_TRUE(original.empty());
    EXPECT_EQ(original.size(), 0u);
    EXPECT_FALSE(original.contains(20));

    EXPECT_TRUE(original.insert(30));
    EXPECT_TRUE(original.contains(30));
}

TEST(AVLTreeMove, AssignmentReplacesOldElements) {
    AVLTree<int> source;
    source.insert(10);
    source.insert(20);

    AVLTree<int> target;
    target.insert(99);

    target = std::move(source);

    EXPECT_EQ(target.size(), 2u);
    EXPECT_TRUE(target.contains(10));
    EXPECT_TRUE(target.contains(20));
    EXPECT_FALSE(target.contains(99));

    EXPECT_TRUE(source.empty());
    EXPECT_EQ(source.size(), 0u);
}

TEST(AVLTreeClear, RemovesAllElementsAndAllowsReuse) {
    AVLTree<int> tree;
    tree.insert(10);
    tree.insert(20);

    tree.clear();

    EXPECT_TRUE(tree.empty());
    EXPECT_EQ(tree.size(), 0u);
    EXPECT_FALSE(tree.contains(10));

    tree.clear(); // Повторная очистка допустима.

    EXPECT_TRUE(tree.insert(30));
    EXPECT_EQ(tree.size(), 1u);
}

TEST(AVLTreeWhere, SelectsMatchingElements) {
    AVLTree<int> tree;
    tree.insert(10);
    tree.insert(20);
    tree.insert(30);

    auto filtered = tree.where([](int value) {
        return value > 15;
    });

    EXPECT_EQ(filtered.size(), 2u);
    EXPECT_FALSE(filtered.contains(10));
    EXPECT_TRUE(filtered.contains(20));
    EXPECT_TRUE(filtered.contains(30));

    EXPECT_EQ(tree.size(), 3u);
    EXPECT_TRUE(tree.contains(10));
}

TEST(AVLTreeWhere, ReturnsEmptyWhenNothingMatches) {
    AVLTree<int> tree;
    tree.insert(10);

    auto filtered = tree.where([](int value) {
        return value < 0;
    });

    EXPECT_TRUE(filtered.empty());
}

TEST(AVLTreeReduce, AddsElementsToInitialValue) {
    AVLTree<int> tree;
    tree.insert(20);
    tree.insert(10);
    tree.insert(30);

    int result = tree.reduce([](int sum, int value) {
        return sum + value;
    }, 100);

    EXPECT_EQ(result, 160);
    EXPECT_EQ(tree.size(), 3u);
}

TEST(AVLTreeReduce, ReturnsInitialForEmptyTree) {
    AVLTree<int> tree;

    int result = tree.reduce([](int sum, int value) {
        return sum + value;
    }, 42);

    EXPECT_EQ(result, 42);
}

TEST(AVLTreeReduce, UsesInorderTraversal) {
    AVLTree<int> tree;
    tree.insert(20);
    tree.insert(10);
    tree.insert(30);

    auto result = tree.reduce(
        [](std::string text, int value) {
            return text + std::to_string(value) + " ";
        },
        std::string{}
    );

    EXPECT_EQ(result, "10 20 30 ");
}