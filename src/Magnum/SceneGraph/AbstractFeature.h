#ifndef Magnum_SceneGraph_AbstractFeature_h
#define Magnum_SceneGraph_AbstractFeature_h
/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015
              Vladimír Vondruš <mosra@centrum.cz>

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/

/** @file
 * @brief Class @ref Magnum::SceneGraph::AbstractFeature, alias @ref Magnum::SceneGraph::AbstractBasicFeature2D, @ref Magnum::SceneGraph::AbstractBasicFeature3D, typedef @ref Magnum::SceneGraph::AbstractFeature2D, @ref Magnum::SceneGraph::AbstractFeature3D, enum @ref Magnum::SceneGraph::CachedTransformation, enum set @ref Magnum::SceneGraph::CachedTransformations
 */

#include <Corrade/Containers/EnumSet.h>
#include <Corrade/Containers/LinkedList.h>

#include "Magnum/Magnum.h"
#include "Magnum/SceneGraph/AbstractObject.h"

namespace Magnum { namespace SceneGraph {

/**
@brief Which transformation to cache in given feature

@see @ref scenegraph-features-caching, @ref CachedTransformations,
    @ref AbstractFeature::setCachedTransformations(), @ref AbstractFeature::clean(),
    @ref AbstractFeature::cleanInverted()
@todo Provide also simpler representations from which could benefit
    other transformation implementations, as they won't need to
    e.g. create transformation matrix from quaternion?
 */
enum class CachedTransformation: UnsignedByte {
    /**
     * Absolute transformation is cached.
     *
     * If enabled, @ref AbstractFeature::clean() is called when cleaning
     * object.
     */
    Absolute = 1 << 0,

    /**
     * Inverted absolute transformation is cached.
     *
     * If enabled, @ref AbstractFeature::cleanInverted() is called when
     * cleaning object.
     */
    InvertedAbsolute = 1 << 1
};

/**
@brief Which transformations to cache in this feature

@see @ref scenegraph-features-caching, @ref AbstractFeature::setCachedTransformations(),
    @ref AbstractFeature::clean(), @ref AbstractFeature::cleanInverted()
*/
typedef Containers::EnumSet<CachedTransformation, UnsignedByte> CachedTransformations;

CORRADE_ENUMSET_OPERATORS(CachedTransformations)

/**
@brief Base for object features

Contained in @ref Object, takes care of transformation caching. See
@ref scenegraph for introduction.

Uses @ref Corrade::Containers::LinkedList for accessing holder object and
sibling features.

## Subclassing

Feature is templated on dimension count and underlying transformation type, so
it can be used only on object having transformation with the same dimension
count and type.

### Caching transformations in features

Features can cache absolute transformation of the object instead of computing
it from scratch every time to achieve better performance. See
@ref scenegraph-features-caching for introduction.

In order to have caching, you must enable it first, because by default the
caching is disabled. You can enable it using @ref setCachedTransformations()
and then implement corresponding cleaning function(s) -- either @ref clean(),
@ref cleanInverted() or both. Example:
@code
class CachingFeature: public SceneGraph::AbstractFeature3D {
    public:
        explicit CachingFeature(SceneGraph::AbstractObject3D& object): SceneGraph::AbstractFeature3D{object} {
            setCachedTransformations(CachedTransformation::Absolute);
        }

    private:
        void clean(const Matrix4& absoluteTransformationMatrix) override {
            _absolutePosition = absoluteTransformationMatrix.translation();
        }

        Vector3 _absolutePosition;
};
@endcode

Before using the cached value explicitly request object cleaning by calling
`object()->setClean()`.

### Accessing object transformation

The feature has by default only access to @ref AbstractObject, which doesn't
know about any used transformation. By using small template trick in the
constructor it is possible to gain access to transformation interface in the
constructor:
@code
class TransformingFeature: public SceneGraph::AbstractFeature3D {
    public:
        template<class T> explicit TransformingFeature(SceneGraph::Object<T>& object):
            SceneGraph::AbstractFeature3D{object}, _transformation{object} {}

    private:
        SceneGraph::AbstractTranslationRotation3D& _transformation;
};
@endcode

See @ref scenegraph-features-transformation for more detailed information.

## Explicit template specializations

The following specializations are explicitly compiled into @ref SceneGraph
library. For other specializations (e.g. using @ref Magnum::Double "Double" type)
you have to use @ref AbstractFeature.hpp implementation file to avoid linker
errors. See also @ref compilation-speedup-hpp for more information.

-   @ref AbstractFeature2D
-   @ref AbstractFeature3D

@see @ref AbstractBasicFeature2D, @ref AbstractBasicFeature3D,
    @ref AbstractFeature2D, @ref AbstractFeature3D
*/
template<UnsignedInt dimensions, class T> class AbstractFeature
    #ifndef DOXYGEN_GENERATING_OUTPUT
    : private Containers::LinkedListItem<AbstractFeature<dimensions, T>, AbstractObject<dimensions, T>>
    #endif
{
    /* GCC 4.6 needs the class keyword */
    friend class Containers::LinkedList<AbstractFeature<dimensions, T>>;
    friend class Containers::LinkedListItem<AbstractFeature<dimensions, T>, AbstractObject<dimensions, T>>;
    template<class> friend class Object;

    public:
        /**
         * @brief Constructor
         * @param object    Object holding this feature
         */
        explicit AbstractFeature(AbstractObject<dimensions, T>& object);

        #ifndef DOXYGEN_GENERATING_OUTPUT
        /* This is here to avoid ambiguity with deleted copy constructor when
           passing `*this` from class subclassing both AbstractFeature and
           AbstractObject */
        template<class U, class = typename std::enable_if<std::is_base_of<AbstractObject<dimensions, T>, U>::value>::type> AbstractFeature(U& object)
        #ifndef CORRADE_GCC46_COMPATIBILITY
        : AbstractFeature(static_cast<AbstractObject<dimensions, T>&>(object)) {}
        #else
        {
            object.Containers::template LinkedList<AbstractFeature<dimensions, T>>::insert(this);
        }
        #endif
        #endif

        virtual ~AbstractFeature() = 0;

        /** @brief Object holding this feature */
        AbstractObject<dimensions, T>& object() {
            return *Containers::LinkedListItem<AbstractFeature<dimensions, T>, AbstractObject<dimensions, T>>::list();
        }

        /** @overload */
        const AbstractObject<dimensions, T>& object() const {
            return *Containers::LinkedListItem<AbstractFeature<dimensions, T>, AbstractObject<dimensions, T>>::list();
        }

        /** @brief Previous feature or `nullptr`, if this is first feature */
        AbstractFeature<dimensions, T>* previousFeature() {
            return Containers::LinkedListItem<AbstractFeature<dimensions, T>, AbstractObject<dimensions, T>>::previous();
        }

        /** @overload */
        const AbstractFeature<dimensions, T>* previousFeature() const {
            return Containers::LinkedListItem<AbstractFeature<dimensions, T>, AbstractObject<dimensions, T>>::previous();
        }

        /** @brief Next feature or `nullptr`, if this is last feature */
        AbstractFeature<dimensions, T>* nextFeature() {
            return Containers::LinkedListItem<AbstractFeature<dimensions, T>, AbstractObject<dimensions, T>>::next();
        }

        /** @overload */
        const AbstractFeature<dimensions, T>* nextFeature() const {
            return Containers::LinkedListItem<AbstractFeature<dimensions, T>, AbstractObject<dimensions, T>>::next();
        }

        /**
         * @{ @name Transformation caching
         *
         * See @ref scenegraph-features-caching for more information.
         */

        /**
         * @brief Which transformations are cached
         *
         * @see @ref scenegraph-features-caching, @ref clean(),
         *      @ref cleanInverted()
         */
        CachedTransformations cachedTransformations() const {
            return _cachedTransformations;
        }

    protected:
        /**
         * @brief Set transformations to be cached
         *
         * Based on which transformation types are enabled, @ref clean() or
         * @ref cleanInverted() is called when cleaning absolute object
         * transformation.
         *
         * Nothing is enabled by default.
         * @see @ref scenegraph-features-caching
         */
        void setCachedTransformations(CachedTransformations transformations) {
            _cachedTransformations = transformations;
        }

        /**
         * @brief Mark feature as dirty
         *
         * Reimplement only if you want to invalidate some external data when
         * object is marked as dirty. All expensive computations should be
         * done in @ref clean() and @ref cleanInverted().
         *
         * Default implementation does nothing.
         * @see @ref scenegraph-features-caching
         */
        virtual void markDirty();

        /**
         * @brief Clean data based on absolute transformation
         *
         * When object is cleaned and @ref CachedTransformation::Absolute is
         * enabled in @ref setCachedTransformations(), this function is called
         * to recalculate data based on absolute object transformation.
         *
         * Default implementation does nothing.
         * @see @ref scenegraph-features-caching, @ref cleanInverted()
         */
        virtual void clean(const typename DimensionTraits<dimensions, T>::MatrixType& absoluteTransformationMatrix);

        /**
         * @brief Clean data based on inverted absolute transformation
         *
         * When object is cleaned and @ref CachedTransformation::InvertedAbsolute
         * is enabled in @ref setCachedTransformations(), this function is
         * called to recalculate data based on inverted absolute object
         * transformation.
         *
         * Default implementation does nothing.
         * @see @ref scenegraph-features-caching, @ref clean()
         */
        virtual void cleanInverted(const typename DimensionTraits<dimensions, T>::MatrixType& invertedAbsoluteTransformationMatrix);

        /*@}*/

    private:
        CachedTransformations _cachedTransformations;
};

#ifndef CORRADE_GCC46_COMPATIBILITY
/**
@brief Base feature for two-dimensional scenes

Convenience alternative to `AbstractFeature<2, T>`. See
@ref AbstractFeature for more information.
@note Not available on GCC < 4.7. Use <tt>%AbstractFeature<2, T></tt> instead.
@see @ref AbstractFeature2D, @ref AbstractBasicFeature3D
*/
#ifndef CORRADE_MSVC2013_COMPATIBILITY /* Apparently cannot have multiply defined aliases */
template<class T> using AbstractBasicFeature2D = AbstractFeature<2, T>;
#endif
#endif

/**
@brief Base feature for two-dimensional float scenes

@see @ref AbstractFeature3D
*/
#ifndef CORRADE_GCC46_COMPATIBILITY
#ifndef CORRADE_MSVC2013_COMPATIBILITY /* Apparently cannot have multiply defined aliases */
typedef AbstractBasicFeature2D<Float> AbstractFeature2D;
#endif
#else
typedef AbstractFeature<2, Float> AbstractFeature2D;
#endif

#ifndef CORRADE_GCC46_COMPATIBILITY
/**
@brief Base feature for three-dimensional scenes

Convenience alternative to `AbstractFeature<3, T>`. See
@ref AbstractFeature for more information.
@note Not available on GCC < 4.7. Use <tt>%AbstractFeature<3, T></tt> instead.
@see @ref AbstractFeature3D, @ref AbstractBasicFeature2D
*/
#ifndef CORRADE_MSVC2013_COMPATIBILITY /* Apparently cannot have multiply defined aliases */
template<class T> using AbstractBasicFeature3D = AbstractFeature<3, T>;
#endif
#endif

/**
@brief Base feature for three-dimensional float scenes

@see @ref AbstractFeature2D
*/
#ifndef CORRADE_GCC46_COMPATIBILITY
#ifndef CORRADE_MSVC2013_COMPATIBILITY /* Apparently cannot have multiply defined aliases */
typedef AbstractBasicFeature3D<Float> AbstractFeature3D;
#endif
#else
typedef AbstractFeature<3, Float> AbstractFeature3D;
#endif

#if defined(CORRADE_TARGET_WINDOWS) && !defined(__MINGW32__)
extern template class MAGNUM_SCENEGRAPH_EXPORT AbstractFeature<2, Float>;
extern template class MAGNUM_SCENEGRAPH_EXPORT AbstractFeature<3, Float>;
#endif

}}

#endif
