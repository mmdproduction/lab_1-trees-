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

TEST(AVLTreeSubtree, ExtractsIndependentSubtree) {
    AVLTree<int> tree;

    for (int value : {20, 10, 30, 5, 15}) {
        tree.insert(value);
    }

    auto subtree = tree.getSubtree(10);

    EXPECT_EQ(subtree.size(), 3u);
    EXPECT_TRUE(subtree.contains(10));
    EXPECT_TRUE(subtree.contains(5));
    EXPECT_TRUE(subtree.contains(15));
    EXPECT_FALSE(subtree.contains(20));

    subtree.erase(5);

    EXPECT_TRUE(tree.contains(5));
    EXPECT_EQ(tree.size(), 5u);
}

TEST(AVLTreeSubtree, ThrowsForMissingRoot) {
    AVLTree<int> tree;
    tree.insert(20);

    EXPECT_THROW(tree.getSubtree(99), std::out_of_range);
}

TEST(AVLTreeSubtree, FindsExtractedSubtree) {
    AVLTree<int> tree;

    for (int value : {20, 10, 30, 5, 15}) {
        tree.insert(value);
    }

    auto subtree = tree.getSubtree(10);

    EXPECT_TRUE(tree.containsSubtree(subtree));
}

TEST(AVLTreeSubtree, RejectsDifferentStructure) {
    AVLTree<int> tree;
    tree.insert(20);
    tree.insert(10);
    tree.insert(30);

    AVLTree<int> subtree;
    subtree.insert(20);
    subtree.insert(10);

    EXPECT_FALSE(tree.containsSubtree(subtree));
}

TEST(AVLTreeSubtree, ContainsEmptySubtree) {
    AVLTree<int> tree;
    AVLTree<int> empty;

    EXPECT_TRUE(tree.containsSubtree(empty));

    tree.insert(10);

    EXPECT_TRUE(tree.containsSubtree(empty));
    EXPECT_FALSE(empty.containsSubtree(tree));
}

TEST(AVLTreeSerialize, EmptyTreeProducesEmptyString) {
    AVLTree<int> tree;
    EXPECT_EQ(tree.serialize(), "");
}

TEST(AVLTreeSerialize, SingleNodeHasNoBranches) {
    AVLTree<int> tree;
    tree.insert(20);
    EXPECT_EQ(tree.serialize(), "{20}");
}

TEST(AVLTreeSerialize, OmitsBranchesOfBothLeaves) {
    AVLTree<int> tree;
    for (int value : {20, 10, 30}) tree.insert(value);
    EXPECT_EQ(tree.serialize(), "{20}({10})[{30}]");
}

TEST(AVLTreeSerialize, PreservesMissingRightChildOfInternalNode) {
    AVLTree<int> tree;
    tree.insert(20);
    tree.insert(10);
    EXPECT_EQ(tree.serialize(), "{20}({10})[]");
}

TEST(AVLTreeSerialize, PreservesMissingLeftChildOfInternalNode) {
    AVLTree<int> tree;
    tree.insert(20);
    tree.insert(30);
    EXPECT_EQ(tree.serialize(), "{20}()[{30}]");
}

TEST(AVLTreeSerialize, PreservesNestedStructure) {
    AVLTree<int> tree;
    for (int value : {20, 10, 30, 15}) tree.insert(value);
    EXPECT_EQ(tree.serialize(), "{20}({10}()[{15}])[{30}]");
}

TEST(AVLTreeSerialize, SerializesTreeAfterRotation) {
    AVLTree<int> tree;
    for (int value : {30, 20, 10}) tree.insert(value);
    EXPECT_EQ(tree.serialize(), "{20}({10})[{30}]");
}

TEST(AVLTreeSerialize, SerializesNegativeValues) {
    AVLTree<int> tree;
    for (int value : {-20, -30, -10}) tree.insert(value);
    EXPECT_EQ(tree.serialize(), "{-20}({-30})[{-10}]");
}

TEST(AVLTreeSerialize, DistinguishesEmptyStringValueFromEmptyTree) {
    AVLTree<std::string> tree;
    tree.insert("");
    EXPECT_EQ(tree.serialize(), "{}");
}

TEST(AVLTreeSerialize, PreservesSpacesAndUnreservedCharacters) {
    AVLTree<std::string> tree;
    tree.insert("hello (){[] world");
    EXPECT_EQ(tree.serialize(), "{hello (){[] world}");
}

TEST(AVLTreeSerialize, EscapesClosingBraceAndBackslash) {
    AVLTree<std::string> tree;
    tree.insert(R"(a}b\c)");
    EXPECT_EQ(tree.serialize(), R"({a\}b\\c})");
}

TEST(AVLTreeSerialize, QuotesAreOrdinaryCharacters) {
    AVLTree<std::string> tree;
    tree.insert("a\"b");
    EXPECT_EQ(tree.serialize(), "{a\"b}");
}

TEST(AVLTreeSerialize, DoesNotModifyTree) {
    AVLTree<int> tree;
    for (int value : {20, 10, 30}) tree.insert(value);
    const auto first = tree.serialize();
    EXPECT_EQ(tree.serialize(), first);
    EXPECT_EQ(tree.size(), 3u);
    for (int value : {20, 10, 30}) EXPECT_TRUE(tree.contains(value));
}

TEST(AVLTreeDeserialize, EmptyInputClearsTree) {
    AVLTree<int> tree;
    tree.insert(99);
    tree.deserialize("");
    EXPECT_TRUE(tree.empty());
    EXPECT_EQ(tree.size(), 0u);
    EXPECT_EQ(tree.serialize(), "");
}

TEST(AVLTreeDeserialize, ReadsLeafWithoutBranches) {
    AVLTree<int> tree;
    tree.deserialize("{20}");
    EXPECT_EQ(tree.size(), 1u);
    EXPECT_TRUE(tree.contains(20));
    EXPECT_EQ(tree.serialize(), "{20}");
}

TEST(AVLTreeDeserialize, RestoresExactStructure) {
    const std::string text = "{20}({10}()[{15}])[{30}]";
    AVLTree<int> tree;
    tree.deserialize(text);
    EXPECT_EQ(tree.size(), 4u);
    EXPECT_EQ(tree.serialize(), text);
    for (int value : {10, 15, 20, 30}) EXPECT_TRUE(tree.contains(value));
}

TEST(AVLTreeDeserialize, ReadsSingleLeftChild) {
    AVLTree<int> tree;
    tree.deserialize("{20}({10})[]");
    EXPECT_EQ(tree.size(), 2u);
    EXPECT_EQ(tree.serialize(), "{20}({10})[]");
}

TEST(AVLTreeDeserialize, ReadsSingleRightChild) {
    AVLTree<int> tree;
    tree.deserialize("{20}()[{30}]");
    EXPECT_EQ(tree.size(), 2u);
    EXPECT_EQ(tree.serialize(), "{20}()[{30}]");
}

TEST(AVLTreeDeserialize, AcceptsFullLegacyLeafBranches) {
    AVLTree<int> tree;
    tree.deserialize("{20}({10}()[])[{30}()[]]");
    EXPECT_EQ(tree.size(), 3u);
    EXPECT_EQ(tree.serialize(), "{20}({10})[{30}]");
}

TEST(AVLTreeDeserialize, AcceptsMixedFullAndShortLeaves) {
    AVLTree<int> tree;
    tree.deserialize("{20}({10})[{30}()[]]");
    EXPECT_EQ(tree.serialize(), "{20}({10})[{30}]");
}

TEST(AVLTreeDeserialize, ReplacesPreviousContents) {
    AVLTree<int> tree;
    tree.insert(99);
    tree.deserialize("{20}({10})[{30}]");
    EXPECT_FALSE(tree.contains(99));
    EXPECT_EQ(tree.size(), 3u);
}

TEST(AVLTreeDeserialize, ReadsEscapesFromLiteral) {
    AVLTree<std::string> tree;
    tree.deserialize(R"({a\}b\\c})");
    EXPECT_EQ(tree.size(), 1u);
    EXPECT_TRUE(tree.contains(R"(a}b\c)"));
    EXPECT_EQ(tree.serialize(), R"({a\}b\\c})");
}

TEST(AVLTreeDeserialize, ReadsEmptyStringValue) {
    AVLTree<std::string> tree;
    tree.deserialize("{}");
    EXPECT_EQ(tree.size(), 1u);
    EXPECT_TRUE(tree.contains(""));
    EXPECT_EQ(tree.serialize(), "{}");
}

TEST(AVLTreeDeserialize, RoundTripPreservesStrings) {
    AVLTree<std::string> original;
    for (const std::string value : {"", "hello (){}[] world", "a}b\\c", "a\"b"})
        original.insert(value);
    AVLTree<std::string> restored;
    restored.deserialize(original.serialize());
    EXPECT_EQ(restored.size(), original.size());
    EXPECT_EQ(restored.serialize(), original.serialize());
    for (const std::string value : {"", "hello (){}[] world", "a}b\\c", "a\"b"})
        EXPECT_TRUE(restored.contains(value));
}

TEST(AVLTreeDeserialize, RoundTripPreservesFiniteDoubleValues) {
    AVLTree<double> original;
    for (double value : {0.1, -1.23456789012345, 1e100}) original.insert(value);
    AVLTree<double> restored;
    restored.deserialize(original.serialize());
    EXPECT_EQ(restored.size(), original.size());
    for (double value : {0.1, -1.23456789012345, 1e100})
        EXPECT_TRUE(restored.contains(value));
    EXPECT_EQ(restored.serialize(), original.serialize());
}

TEST(AVLTreeDeserialize, RejectsInvalidInputWithoutChangingTree) {
    const std::string invalidInputs[] = {
        "junk",                 // Missing opening brace.
        "{20",                  // Missing closing brace.
        "{20}junk",             // Trailing data.
        "{20}{30}",             // Two roots.
        "{20}[]",               // Right branch without left wrapper.
        "{20}()",               // Missing right wrapper.
        "{20}({10})[",          // Unterminated right branch.
        "{20}({10}][{30}]",     // Wrong closing bracket.
        "{12abc}",              // Partially valid number.
        "{}",                   // Empty integer value.
        R"({1\q})",             // Unknown escape.
        std::string("{1") + '\\', // Incomplete escape.
        "{20}({30})[]",         // Left key exceeds root.
        "{20}()[{10}]",         // Right key is below root.
        "{20}({20})[]",         // Duplicate key.
        "{20}({10}()[{25}])[{30}]", // Violates an ancestor bound.
        "{10}()[{20}()[{30}]]"  // BST order is valid, AVL balance is not.
    };
    for (const auto& input : invalidInputs) {
        SCOPED_TRACE(input);
        AVLTree<int> tree;
        tree.insert(99);
        tree.insert(50);
        const auto before = tree.serialize();
        EXPECT_THROW(tree.deserialize(input), std::invalid_argument);
        EXPECT_EQ(tree.serialize(), before);
        EXPECT_EQ(tree.size(), 2u);
        EXPECT_TRUE(tree.contains(99));
        EXPECT_TRUE(tree.contains(50));
    }
}

TEST(AVLTreeDeserialize, RemainsUsableAfterLoading) {
    AVLTree<int> tree;
    tree.deserialize("{20}({10})[{30}]");
    EXPECT_TRUE(tree.insert(25));
    EXPECT_TRUE(tree.erase(10));
    EXPECT_EQ(tree.size(), 3u);
    EXPECT_FALSE(tree.contains(10));
    for (int value : {20, 25, 30}) EXPECT_TRUE(tree.contains(value));
}