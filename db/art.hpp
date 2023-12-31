#pragma once
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string_view>
#include <sys/types.h>
#include <vector>

#include "slice.hpp"

namespace art {

using ARTData = std::vector<uint8_t>;
enum class NodeType : uint8_t { Node4, Node16, Node48, Node256, Leaf };
inline constexpr size_t MAX_PARTIAL_LEN = 10;

// Abstract base class of all type of node. It knows nothing but its type.
class Node {
public:
  virtual ~Node() = default;

  NodeType type;
};
using NodeRef = std::atomic<Node *>;

// Abstract base class of all type of ART inner node, which stores the basic
// info of a key path.
class InnerNode : public Node {
public:
  virtual ~InnerNode() = default;
  
protected:
  uint8_t children_count;
  size_t partial_len;
  std::array<unsigned char, MAX_PARTIAL_LEN> partial_key;
};

// Smallest node type, which can store up to 4 child pointers.
// Keys and pointers are stored at corresponding positions.
// Keys are sorted.
class Node4 : public InnerNode {
public:
  Node* findChild(unsigned char byte);
  void addChild(unsigned char byte, Node* child);

private:
  std::array<unsigned char, 4> keys;
  std::array<NodeRef, 4> children;
};

// Storing between 5 and 16 child pointers.
// Keys and pointers are stored at corresponding positions.
// Keys are sorted.
class Node16 : public InnerNode {
public:
 Node* findChild(unsigned char byte);
 void addChild(unsigned char byte, Node* child);

private:
  std::array<unsigned char, 16> keys;
  std::array<NodeRef, 16> children;
};

// Store between 17 and 48 child pointers.
// Child pointers can be indexed directly by key.
class Node48 : public InnerNode {
public:
  Node* findChild(unsigned char byte);
  void addChild(unsigned char byte, Node* child);

private:
  std::array<unsigned char, 256> keys;
  std::array<NodeRef, 48> children;
};

// Store between 49 and 256 child pointers.
// Key is the index or array `children`, child node can be found
// by a single lookup.
class Node256 : public InnerNode {
public:
  Node* findChild(unsigned char byte);
  void addChild(unsigned char byte, Node* child);

private:
  std::array<NodeRef, 256> children;
};

// Leaf node which contains complete key/value data.
class LeafNode : public Node {
private:
  art::ARTData key;
  art::ARTData val;
};

/**
 * @class ART
 * @brief Adaptive Radix Tree (ART) class implementation.
 *
 * The ART is a type of radix tree optimized for fast and space-efficient
 * key-value storage and retrieval. It is particularly efficient for database
 * indexing. ART can also more fully utilize modern CPU features, such as
 * multi-core processors and SIMD instructions.
 *
 * @details
 * The ART class provides methods to search, insert, and remove key-value pairs.
 * - The `search` method looks for a value associated with a given key and
 * returns an optional result.
 * - The `insert` method adds a new key-value pair to the tree.
 * - The `remove` method deletes a key-value pair from the tree based on the
 * key.
 *
 * The tree dynamically adjusts the node types as keys are added or removed to
 * provide space and performance efficiency.
 *
 * @note This implementation assumes unique keys for insertion.
 *
 * Usage example:
 * @code
 *     ART tree;
 *     tree.insert("key1", "value1");
 *     auto value = tree.search("key1");
 *     if (value) {
 *         std::cout << "Found value: " << *value << std::endl;
 *     }
 *     tree.remove("key1");
 * @endcode
 */
class ART {
public:
  /**
   * Inserts a new key-value pair into the ART.
   *
   * @param key key The key to insert, which is a byte stream.
   * @param value value The value to associate with the key, which is a byte
   * stream. This value is moved into the tree.
   */
  void insert(Slice key, OwnedSlice value);

  /**
   * Searches for a value associated with a given key in the ART.
   *
   * @param key the key for which to search. Before using it, you should convert
   * the object to a byte stream
   * @return std::optional<ARTDataRef>
   */
  std::optional<std::span<uint8_t>> search(Slice key);

  /**
   * Removes a key-value pair from the ART, identified by the key.
   *
   * @param key The key of the pair to remove. Before using it, you should
   * convert the object to a byte stream
   */
  void remove(Slice key);

  /**
   * Returns the tree size.
   */
  size_t size();

private:
  NodeRef root;
  size_t tree_size;

  void insertRecursively(NodeRef node, std::span<uint8_t> key, LeafNode leaf, int depth);
  static void replace(NodeRef& node, Node* newNode);
};

} // namespace art

