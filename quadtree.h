// ================================= //
//                                   //
// QuadTree Version 0.1a             //
// Copyright (c) 2017 NuclearC       //
//                                   //
// ================================= //

// quadtree.h: Header only implementation

#ifndef NC_QUADTREE_H_
#define NC_QUADTREE_H_

#include <vector>
#include <memory>

namespace nc {
    template <typename T>
    struct QuadTreeAABB {
        // boundaries
        T left, top, right, bottom;
        // center
        T x, y;
        // dimensions
        T width, height;

        void set_dimensions() {
            width = bottom - top;
            height = right - left;
        }

        void set_center() {
            x = (left + right) / (T)2;
            y = (top + bottom) / (T)2;
        }

        bool verify() const {
            return ((left < right) && (top < bottom));
        }

        bool intersects(const QuadTreeAABB<T>& _Other) const {
            return (left < _Other.right &&
                right > _Other.left &&
                top < _Other.bottom &&
                bottom > _Other.top);
        }

        bool contains(T _X, T _Y) const {
            return ((_X > left) && (_X < right) 
                && (_Y > top) && (_Y < bottom));
        }
    };

    template <typename T>
    struct QuadTreeObject {
        QuadTreeAABB<T> bounds;
        void* user_data;

        bool removed = true;

        // unique id used to remove the object later
        size_t id = 0;
    };

    template <typename T = double, size_t _Capacity = 2>
    class QuadTree 
        : public std::enable_shared_from_this<QuadTree<T,_Capacity>> {
    private:
        static constexpr size_t kChildren = 4;

        QuadTreeAABB<T> bounds;
        QuadTreeAABB<T> max_bounds;

        size_t object_count = 0;
        std::vector<QuadTreeObject<T>> objects;
        std::vector<std::shared_ptr<QuadTree<T,_Capacity>>> children;
        std::shared_ptr<QuadTree<T, _Capacity>> root;

        bool has_children = false;

        void split();
        void merge();

        void remove_empty_nodes();

        size_t level = 1;
    public:
        QuadTree() {
            level = 0;

            objects.resize(_Capacity);
            has_children = false;

            for (size_t i = 0; i < _Capacity; i++)
                objects[i].removed = true;
        }

        QuadTree(const QuadTreeAABB<T>& _Bounds) : bounds(_Bounds) {
            level = 0;

            objects.resize(_Capacity);
            has_children = false;

            for (size_t i = 0; i < _Capacity; i++)
                objects[i].removed = true;
        }

        ~QuadTree() {
        }

        void set_bounds(const QuadTreeAABB<T>& _Bounds) {
            bounds = _Bounds;
        }

        void resolve_max_bounds();

        const QuadTreeAABB<T>& get_bounds() const {
            return bounds;
        }

        bool insert(const QuadTreeObject<T>& _Object);
        bool remove(const QuadTreeObject<T>& _Object);

        void query(const QuadTreeAABB<T>& _Boundaries,
            QuadTreeObject<T>* _Objects, size_t& _Length,
            bool _BoundChecks = true) const;
        void query(const QuadTreeAABB<T>& _Boundaries,
            std::vector<QuadTreeObject<T>>& _Objects,
            bool _BoundChecks = true) const;

        bool has_children_() const { return has_children; }

        const std::vector<std::shared_ptr<QuadTree<T, _Capacity>>>& get_children() const { return children; }

        size_t get_total_objects() const;
    };

    template<typename T, size_t _Capacity>
    inline void QuadTree<T, _Capacity>::split()
    {
        if (!has_children) {
            QuadTreeAABB<T> child_aabb;

            children.resize(kChildren);
            // top left
            child_aabb.left = bounds.left;
            child_aabb.top = bounds.top;
            child_aabb.right = bounds.x;
            child_aabb.bottom = bounds.y;
            child_aabb.set_center();
            child_aabb.set_dimensions();
            children[0] = std::make_shared<QuadTree<T, _Capacity>>(child_aabb);
            children[0]->level = level + 1;
            children[0]->root = shared_from_this();
            // top right
            child_aabb.left = bounds.x;
            child_aabb.top = bounds.top;
            child_aabb.right = bounds.right;
            child_aabb.bottom = bounds.y;
            child_aabb.set_center();
            child_aabb.set_dimensions();
            children[1] = std::make_shared<QuadTree<T, _Capacity>>(child_aabb);
            children[1]->set_bounds(child_aabb);
            children[1]->level = level + 1;
            children[1]->root = shared_from_this();
            // bottom right
            child_aabb.left = bounds.x;
            child_aabb.top = bounds.y;
            child_aabb.right = bounds.right;
            child_aabb.bottom = bounds.bottom;
            child_aabb.set_center();
            child_aabb.set_dimensions();
            children[2] = std::make_shared<QuadTree<T, _Capacity>>(child_aabb);
            children[2]->set_bounds(child_aabb);
            children[2]->level = level + 1;
            children[2]->root = shared_from_this();
            // bottom left
            child_aabb.left = bounds.left;
            child_aabb.top = bounds.y;
            child_aabb.right = bounds.x;
            child_aabb.bottom = bounds.bottom;
            child_aabb.set_center();
            child_aabb.set_dimensions();
            children[3] = std::make_shared<QuadTree<T, _Capacity>>(child_aabb);
            children[3]->set_bounds(child_aabb);
            children[3]->level = level + 1;
            children[3]->root = shared_from_this();

            has_children = true;
        }
    }

    template<typename T, size_t _Capacity>
    inline void QuadTree<T, _Capacity>::merge()
    {
        if (has_children) {
            children[0]->merge();
            children[1]->merge();
            children[2]->merge();
            children[3]->merge();

            children.clear();

            has_children = false;
        }
    }

    template<typename T, size_t _Capacity>
    inline void QuadTree<T, _Capacity>::remove_empty_nodes()
    {
        if (has_children) {
            children[0]->remove_empty_nodes();
            children[1]->remove_empty_nodes();
            children[2]->remove_empty_nodes();
            children[3]->remove_empty_nodes();

            if (get_total_objects() < 1)
                merge();
        }
    }

    template<typename T, size_t _Capacity>
    inline void QuadTree<T, _Capacity>::resolve_max_bounds()
    {
        max_bounds = bounds;

        for (size_t i = 0; i < _Capacity; i++) {
            if (!objects[i].removed) {
                max_bounds.left = std::min(bounds.left, objects[i].bounds.left);
                max_bounds.top = std::min(bounds.top, objects[i].bounds.top);
                max_bounds.right = std::max(bounds.right, objects[i].bounds.right);
                max_bounds.bottom = std::max(bounds.bottom, objects[i].bounds.bottom);

                if (!max_bounds.verify()) {
                    max_bounds = bounds;
                }
            }
        }

        if (has_children) {
            for (size_t i = 0; i < kChildren; i++) {
                max_bounds.left = std::min(max_bounds.left, children[i]->max_bounds.left);
                max_bounds.top = std::min(max_bounds.top, children[i]->max_bounds.top);
                max_bounds.right = std::max(max_bounds.right, children[i]->max_bounds.right);
                max_bounds.bottom = std::max(max_bounds.bottom, children[i]->max_bounds.bottom);

                if (!max_bounds.verify()) {
                    max_bounds = bounds;
                }
            }
        }

        if (root != nullptr) {
            root->resolve_max_bounds();
        }
    }

    template<typename T, size_t _Capacity>
    inline bool QuadTree<T, _Capacity>::insert(const QuadTreeObject<T>& _Object)
    {
        if (bounds.intersects(_Object.bounds)) {
            if (object_count >= _Capacity) {
                if (!has_children)
                    split();

                if (!children[0]->insert(_Object) 
                    && !children[1]->insert(_Object)
                    && !children[2]->insert(_Object)
                    && !children[3]->insert(_Object))
                    throw std::out_of_range("object position out of range");

                return true;
            }
            else {
                for (size_t i = 0; i < _Capacity; i++) {
                    if (objects[i].removed) {
                        objects[i].bounds = _Object.bounds;
                        objects[i].user_data = _Object.user_data;
                        objects[i].id = _Object.id;
                        objects[i].removed = false;

                        resolve_max_bounds();

                        object_count++;
                        return true;
                    }
                }
            }
        }

        return false;
    }

    template<typename T, size_t _Capacity>
    inline bool QuadTree<T, _Capacity>::remove(const QuadTreeObject<T>& _Object)
    {
        if (bounds.intersects(_Object.bounds)) {
            if (object_count > 0) {
                for (size_t i = 0; i < _Capacity; i++) {
                    if (!objects[i].removed && objects[i].id == _Object.id) {
                        objects[i].removed = true;
                        object_count--;
                        remove_empty_nodes();

                        resolve_max_bounds();

                        return true;
                    }
                }
            }
            
            if (has_children) {
                if (!children[0]->remove(_Object)
                    && !children[1]->remove(_Object)
                    && !children[2]->remove(_Object)
                    && !children[3]->remove(_Object))
                    return false;

                return true;
            }

        }
        else {
            return false;
        }
    }

    template<typename T, size_t _Capacity>
    inline void QuadTree<T, _Capacity>::query(const QuadTreeAABB<T>& _Bounds, 
        QuadTreeObject<T>* _Objects, size_t& _Length, bool _BoundChecks) const
    {
        if (max_bounds.intersects(_Bounds) || !_BoundChecks) {
            if (has_children) {
                children[0]->query(_Bounds, _Objects, _Length, _BoundChecks);
                children[1]->query(_Bounds, _Objects, _Length, _BoundChecks);
                children[2]->query(_Bounds, _Objects, _Length, _BoundChecks);
                children[3]->query(_Bounds, _Objects, _Length, _BoundChecks);
            }

            if (object_count < 1)
                return;

            for (size_t i = 0; i < _Capacity; i++) {
                if (!objects[i].removed 
                    && objects[i].bounds.intersects(_Bounds)) {
                    _Objects[_Length++] = objects[i];
                }
            }
        }
    }

    template<typename T, size_t _Capacity>
    inline void QuadTree<T, _Capacity>::query(const QuadTreeAABB<T>& _Bounds,
        std::vector<QuadTreeObject<T>>& _Objects, bool _BoundChecks) const
    {
        if (bounds.intersects(_Bounds) || !_BoundChecks) {
            if (has_children) {
                children[0]->query(_Bounds, _Objects, _BoundChecks);
                children[1]->query(_Bounds, _Objects, _BoundChecks);
                children[2]->query(_Bounds, _Objects, _BoundChecks);
                children[3]->query(_Bounds, _Objects, _BoundChecks);
            }

            if (object_count < 1)
                return;

            for (size_t i = 0; i < _Capacity; i++) {
                if (!objects[i].removed
                    && objects[i].bounds.intersects(_Bounds)) {
                    _Objects.push_back(objects[i]);
                }
            }
        }
    }

    template<typename T, size_t _Capacity>
    inline size_t QuadTree<T, _Capacity>::get_total_objects() const
    {
        size_t obj_count = object_count;

        if (has_children) {
            obj_count += children[0]->get_total_objects() +
                children[1]->get_total_objects() +
                children[2]->get_total_objects() +
                children[3]->get_total_objects();
        }

        return obj_count;
    }
} // namespace nc

#endif // NC_QUADTREE_H_