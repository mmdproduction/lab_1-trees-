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