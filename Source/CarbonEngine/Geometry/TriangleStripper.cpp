/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Geometry/TriangleStripper.h"
#include "CarbonEngine/Math/MathCommon.h"

namespace Carbon
{

const auto CacheSize = 16U;

class VertexCache
{
public:

    VertexCache(unsigned int size) : entries_(size, -1) {}

    bool has(int entry) const { return entries_.has(entry); }

    void add(int entry)
    {
        if (!has(entry))
        {
            entries_.popBack();
            entries_.prepend(entry);
        }
    }

private:

    Vector<int> entries_;
};

class StripInfo;

class FaceInfo
{
public:

    FaceInfo(unsigned int v0, unsigned int v1, unsigned int v2, bool isFake_ = false) : v{{v0, v1, v2}}, isFake(isFake_) {}

    std::array<unsigned int, 3> v = {};
    StripInfo* strip = nullptr;
    StripInfo* testStrip = nullptr;
    int experimentID = -1;    // In what experiment was it given an experiment ID ?
    bool isFake = false;      // If true, will be deleted when the strip it's in is deleted

    bool isDegenerate() const { return v[0] == v[1] || v[0] == v[2] || v[1] == v[2]; }
};

// Dumb edge class that knows its indices, its two faces, and the next edge using the lesser of the indices
class EdgeInfo
{
public:

    EdgeInfo(unsigned int v0, unsigned int v1) : v{{v0, v1}}, referenceCount_(2) {}

    void release()
    {
        if (--referenceCount_ == 0)
            delete this;
    }

    std::array<unsigned int, 2> v = {};
    std::array<FaceInfo*, 2> face = {};
    std::array<EdgeInfo*, 2> nextV = {};

private:

    unsigned int referenceCount_ = 2;
};

// This class is a quick summary of parameters used to begin a triangle strip. Some operations create lists of these.
struct StripStartInfo
{
    StripStartInfo(FaceInfo* startFace_, EdgeInfo* startEdge_, bool toV1_)
        : startFace(startFace_), startEdge(startEdge_), toV1(toV1_)
    {
    }

    FaceInfo* startFace = nullptr;
    EdgeInfo* startEdge = nullptr;
    bool toV1 = false;
};

// This is a summary of a strip that has been built
class StripInfo : private Noncopyable
{
public:

    StripInfo(const StripStartInfo& startInfo_, int experimentID_ = -1) : startInfo(startInfo_), experimentID(experimentID_) {}

    // This is an experiment if the experiment id is >= 0
    bool isExperiment() const { return experimentID >= 0; }

    bool hasFace(const FaceInfo* faceInfo) const
    {
        return faceInfo && (experimentID >= 0 ? faceInfo->testStrip == this : faceInfo->strip == this);
    }

    bool sharesEdge(const FaceInfo* faceInfo, Vector<EdgeInfo*>& edgeInfos);

    // Mark the triangle as taken by this strip
    bool isMarked(const FaceInfo* faceInfo);
    void markTriangle(FaceInfo* faceInfo);

    // Build the strip
    void build(Vector<EdgeInfo*>& edgeInfos, Vector<FaceInfo*>& faceInfos);

    Vector<FaceInfo*> faces;
    StripStartInfo startInfo;
    int experimentID = -1;

    bool visited = false;

    unsigned int degenerateCount = 0;
};

class Stripifier
{
public:

    bool stripify(const Vector<unsigned int>& indices, unsigned int maxIndex, Vector<StripInfo*>& outStrips,
                  Vector<FaceInfo*>& outFaceList, Runnable& r);
    void createStrips(const Vector<StripInfo*>& allStrips, Vector<int>& stripIndices, bool stitchStrips,
                      unsigned int& stripCount);

    static int getUniqueVertexInB(const FaceInfo* a, const FaceInfo* b);
    static void getSharedVertices(const FaceInfo* a, const FaceInfo* b, int& vertex0, int& vertex1);

    bool isClockwise(const FaceInfo* faceInfo, unsigned int v0, unsigned int v1);
    bool isNextClockwise(unsigned int indexCount);

    static int getNextIndex(const Vector<unsigned int>& indices, const FaceInfo* face);
    static EdgeInfo* findEdgeInfo(Vector<EdgeInfo*>& edgeInfos, unsigned int v0, unsigned int v1);
    static FaceInfo* findOtherFace(Vector<EdgeInfo*>& edgeInfos, unsigned int v0, unsigned int v1, FaceInfo* faceInfo);
    FaceInfo* findGoodResetPoint(Vector<FaceInfo*>& faceInfos, Vector<EdgeInfo*>& edgeInfos);
    bool findAllStrips(Vector<StripInfo*>& allStrips, Vector<FaceInfo*>& allFaceInfos, Vector<EdgeInfo*>& allEdgeInfos,
                       unsigned int sampleCount, Runnable& r);
    bool splitUpStripsAndOptimize(Vector<StripInfo*>& allStrips, Vector<StripInfo*>& outStrips, Vector<EdgeInfo*>& edgeInfos,
                                  Vector<FaceInfo*>& outFaceList, Runnable& r);
    bool findTraversal(Vector<FaceInfo*>& faceInfos, Vector<EdgeInfo*>& edgeInfos, StripInfo* strip, StripStartInfo& startInfo);
    void updateCacheStrip(VertexCache& vertexCache, const StripInfo* strip);
    bool buildstripifyInfo(Vector<FaceInfo*>& faceInfos, Vector<EdgeInfo*>& edgeInfos_, unsigned int maxIndex, Runnable& r);

private:

    Vector<unsigned int> indices_;
    float meshJump_ = 0.0f;
    bool firstTimeResetPoint_ = false;
};

// Find the edge info for these two indices
EdgeInfo* Stripifier::findEdgeInfo(Vector<EdgeInfo*>& edgeInfos, unsigned int v0, unsigned int v1)
{
    // We can get to it through either array because the edge infos have a v0 and v1 and there is no order except how it was
    // first created
    auto infoIter = edgeInfos[v0];

    while (infoIter)
    {
        if (infoIter->v[0] == v0)
        {
            if (infoIter->v[1] == v1)
                return infoIter;

            infoIter = infoIter->nextV[0];
        }
        else
        {
            if (infoIter->v[0] == v1)
                return infoIter;

            infoIter = infoIter->nextV[1];
        }
    }

    return nullptr;
}

// Find the other face sharing these vertices exactly like the edge info above
FaceInfo* Stripifier::findOtherFace(Vector<EdgeInfo*>& edgeInfos, unsigned int v0, unsigned int v1, FaceInfo* faceInfo)
{
    auto edgeInfo = findEdgeInfo(edgeInfos, v0, v1);

    if (!edgeInfo && v0 == v1)
        return nullptr;    // We've hit a degenerate

    return edgeInfo->face[0] == faceInfo ? edgeInfo->face[1] : edgeInfo->face[0];
}

// Builds the list of all face and edge infos
bool Stripifier::buildstripifyInfo(Vector<FaceInfo*>& faceInfos, Vector<EdgeInfo*>& edgeInfos_, unsigned int maxIndex,
                                   Runnable& r)
{
    // Reserve space for the face infos, but do not resize them
    faceInfos.reserve(indices_.size() / 3);

    // Clear edge infos
    edgeInfos_.clear();
    edgeInfos_.resize(maxIndex + 1, nullptr);

    for (auto i = 0U; i < indices_.size(); i += 3)
    {
        auto mightAlreadyExist = true;
        auto faceUpdated = std::array<bool, 3>();

        // Grab the indices
        auto v = std::array<int, 3>{{int(indices_[i]), int(indices_[i + 1]), int(indices_[i + 2])}};

        // Ignore degenerates
        if (FaceInfo(v[0], v[1], v[2]).isDegenerate())
            continue;

        // Create the face info and add it to the list of faces, but only if this exact face doesn't already exist in the list
        auto faceInfo = new FaceInfo(v[0], v[1], v[2]);

        // Grab the edge infos, creating them if they do not already exist
        auto edgeInfos = std::array<EdgeInfo*, 3>();
        for (auto j = 0U; j < 3; j++)
        {
            auto e0 = v[j];
            auto e1 = v[(j + 1) % 3];

            edgeInfos[j] = findEdgeInfo(edgeInfos_, e0, e1);

            if (!edgeInfos[j])
            {
                // Since one of its edges isn't in the edge data structure, it can't already exist in the face structure
                mightAlreadyExist = false;

                // Create the info
                edgeInfos[j] = new EdgeInfo(e0, e1);

                // Update the linked list on both
                edgeInfos[j]->nextV[0] = edgeInfos_[e0];
                edgeInfos[j]->nextV[1] = edgeInfos_[e1];
                edgeInfos_[e0] = edgeInfos[j];
                edgeInfos_[e1] = edgeInfos[j];

                // Set face 0
                edgeInfos[j]->face[0] = faceInfo;
            }
            else
            {
                if (edgeInfos[j]->face[1])
                {
                    static auto isWarningShown = false;
                    if (!isWarningShown)
                        LOG_DEBUG << "TriangleStripper: more than two triangles on an edge, uncertain consequences";
                    isWarningShown = true;
                }
                else
                {
                    edgeInfos[j]->face[1] = faceInfo;
                    faceUpdated[j] = true;
                }
            }
        }

        if (mightAlreadyExist)
        {
            auto alreadyExists = false;
            for (auto f : faceInfos)
            {
                if (f->v == faceInfo->v)
                {
                    alreadyExists = true;
                    break;
                }
            }

            if (!alreadyExists)
                faceInfos.append(faceInfo);
            else
            {
                delete faceInfo;

                // Cleanup pointers that point to this deleted face
                if (faceUpdated[0])
                    edgeInfos[0]->face[1] = nullptr;
                if (faceUpdated[1])
                    edgeInfos[1]->face[1] = nullptr;
                if (faceUpdated[2])
                    edgeInfos[2]->face[1] = nullptr;
            }
        }
        else
            faceInfos.append(faceInfo);

        // Check in with runnable
        if (r.setTaskProgress(i + 3, indices_.size()))
            return false;
    }

    return true;
}

// A good reset point is one near other committed areas so that we know that when we've made the longest strips its because
// we're stripifying in the same general orientation.
FaceInfo* Stripifier::findGoodResetPoint(Vector<FaceInfo*>& faceInfos, Vector<EdgeInfo*>& edgeInfos)
{
    // We hop into different areas of the mesh to try to get other large open spans done. Areas of small strips can just be left
    // to triangle lists added at the end.
    auto result = pointer_to<FaceInfo>::type();

    auto startPoint = -1;
    if (firstTimeResetPoint_)
    {
        firstTimeResetPoint_ = false;

        // First time, find a face with few neighbors (look for an edge of the mesh)
        auto bestValue = 0U;
        for (auto i = 0U; i < faceInfos.size(); i++)
        {
            auto value = 0U;

            if (findOtherFace(edgeInfos, faceInfos[i]->v[0], faceInfos[i]->v[1], faceInfos[i]) == nullptr)
                value++;
            if (findOtherFace(edgeInfos, faceInfos[i]->v[1], faceInfos[i]->v[2], faceInfos[i]) == nullptr)
                value++;
            if (findOtherFace(edgeInfos, faceInfos[i]->v[2], faceInfos[i]->v[0], faceInfos[i]) == nullptr)
                value++;

            if (value > bestValue)
            {
                bestValue = value;
                startPoint = i;
            }
        }
    }

    if (startPoint == -1)
        startPoint = int(float(faceInfos.size() - 1) * meshJump_);

    for (auto i = 0U; i < faceInfos.size(); i++)
    {
        auto face = faceInfos[(startPoint + i) % faceInfos.size()];
        if (!face->strip)
        {
            result = face;
            break;
        }
    }

    // Update the meshJump
    meshJump_ += 0.1f;
    if (meshJump_ > 1.0f)
        meshJump_ = .05f;

    return result;
}

// Returns the vertex unique to b
int Stripifier::getUniqueVertexInB(const FaceInfo* a, const FaceInfo* b)
{
    for (auto index : b->v)
    {
        if (index != a->v[0] && index != a->v[1] && index != a->v[2])
            return index;
    }

    // Nothing is different
    return -1;
}

// Returns the (at most) two vertices shared between the two faces
void Stripifier::getSharedVertices(const FaceInfo* a, const FaceInfo* b, int& vertex0, int& vertex1)
{
    vertex0 = -1;
    vertex1 = -1;

    for (auto index : b->v)
    {
        if (index == a->v[0] || index == a->v[1] || index == a->v[2])
        {
            if (vertex0 == -1)
                vertex0 = index;
            else
            {
                vertex1 = index;
                return;
            }
        }
    }
}

// Returns vertex of the input face which is "next" in the input index list
int Stripifier::getNextIndex(const Vector<unsigned int>& indices, const FaceInfo* face)
{
    auto v = std::array<unsigned int, 2>{{indices[indices.size() - 2], indices.back()}};
    auto fv = std::array<unsigned int, 3>{{face->v[0], face->v[1], face->v[2]}};

    for (auto i = 0U; i < 3; i++)
    {
        if (fv[i] != v[0] && fv[i] != v[1])
        {
            if ((fv[(i + 1) % 3] != v[0] && fv[(i + 1) % 3] != v[1]) || (fv[(i + 2) % 3] != v[0] && fv[(i + 2) % 3] != v[1]))
                LOG_DEBUG << "Triangle doesn't have all of its vertices, duplicate triangle probably got us derailed";

            return fv[i];
        }
    }

    // Shouldn't get here, but let's try and fail gracefully
    if (fv[0] == fv[1] || fv[0] == fv[2])
        return fv[0];

    if (fv[1] == fv[2])
        return fv[1];

    return -1;
}

// If either the faceInfo has a real strip index because it is already assigned to a committed strip OR it is assigned in an
// experiment and the experiment index is the one we are building for, then it is marked and unavailable
bool StripInfo::isMarked(const FaceInfo* faceInfo)
{
    return faceInfo->strip || (isExperiment() && faceInfo->experimentID == experimentID);
}

// Marks the face with the current strip ID
void StripInfo::markTriangle(FaceInfo* faceInfo)
{
    if (isExperiment())
    {
        faceInfo->experimentID = experimentID;
        faceInfo->testStrip = this;
    }
    else
    {
        assert(!faceInfo->strip && "Face is already marked");
        faceInfo->experimentID = -1;
        faceInfo->strip = this;
    }
}

// Builds a strip forward as far as we can go, then builds backwards, and joins the two lists
void StripInfo::build(Vector<EdgeInfo*>& edgeInfos, Vector<FaceInfo*>& faceInfos)
{
    // Used in building the strips forward and backward
    auto scratchIndices = Vector<unsigned int>();

    // Build forward ... start with the initial face
    auto forwardFaces = Vector<FaceInfo*>{startInfo.startFace};
    auto backwardFaces = Vector<FaceInfo*>();
    markTriangle(startInfo.startFace);

    auto v0 = startInfo.startEdge->v[0];
    auto v1 = startInfo.startEdge->v[1];
    if (!startInfo.toV1)
        std::swap(v0, v1);

    // Easiest way to get v2 is to use this function which requires the other indices to already be in the list
    scratchIndices.append(v0);
    scratchIndices.append(v1);
    auto v2 = Stripifier::getNextIndex(scratchIndices, startInfo.startFace);
    scratchIndices.append(v2);

    // Build the forward list
    auto nv0 = v1;
    auto nv1 = v2;
    auto nextFace = Stripifier::findOtherFace(edgeInfos, nv0, nv1, startInfo.startFace);
    while (nextFace && !isMarked(nextFace))
    {
        // Check to see if this next face is going to cause us to die soon
        auto testnv0 = nv1;
        auto testnv1 = Stripifier::getNextIndex(scratchIndices, nextFace);

        auto nextNextFace = Stripifier::findOtherFace(edgeInfos, testnv0, testnv1, nextFace);

        if (!nextNextFace || isMarked(nextNextFace))
        {
            // Uh oh, we're following a dead end, try swapping
            auto testNextFace = Stripifier::findOtherFace(edgeInfos, nv0, testnv1, nextFace);

            if (testNextFace && !isMarked(testNextFace))
            {
                // We only swap if it buys us something

                // Add a "fake" degenerate face
                auto tempFace = new FaceInfo(nv0, nv1, nv0, true);

                forwardFaces.append(tempFace);
                markTriangle(tempFace);

                scratchIndices.append(nv0);
                testnv0 = nv0;

                degenerateCount++;
            }
        }

        // Add this to the strip
        forwardFaces.append(nextFace);
        markTriangle(nextFace);

        // Add the index
        scratchIndices.append(testnv1);

        // And get the next face
        nv0 = testnv0;
        nv1 = testnv1;
        nextFace = Stripifier::findOtherFace(edgeInfos, nv0, nv1, nextFace);
    }

    // tempAllFaces is going to be forwardFaces + backwardFaces
    auto tempAllFaces = forwardFaces;

    // Reset the indices for building the strip backwards and do so
    scratchIndices = Vector<unsigned int>{uint(v2), uint(v1), uint(v0)};
    nv0 = v1;
    nv1 = v0;
    nextFace = Stripifier::findOtherFace(edgeInfos, nv0, nv1, startInfo.startFace);
    while (nextFace && !isMarked(nextFace))
    {
        // This tests to see if newFace is "unique", meaning that its vertices aren't already in the list so, strips which
        // "wrap-around" are not allowed

        auto bv0 = false;
        auto bv1 = false;
        auto bv2 = false;
        for (auto& tempAllFace : tempAllFaces)
        {
            bv0 = bv0 || (tempAllFace->v[0] == nextFace->v[0] || tempAllFace->v[1] == nextFace->v[0] ||
                          tempAllFace->v[2] == nextFace->v[0]);
            bv1 = bv1 || (tempAllFace->v[0] == nextFace->v[1] || tempAllFace->v[1] == nextFace->v[1] ||
                          tempAllFace->v[2] == nextFace->v[1]);
            bv2 = bv2 || (tempAllFace->v[0] == nextFace->v[2] || tempAllFace->v[1] == nextFace->v[2] ||
                          tempAllFace->v[2] == nextFace->v[2]);

            // The face is not unique because all its vertices exist in the face vector
            if (bv0 && bv1 && bv2)
                break;
        }

        // Break out if nextFace is not unique
        if (bv0 && bv1 && bv2)
            break;

        // Check to see if this next face is going to cause us to die soon
        auto testnv0 = nv1;
        auto testnv1 = Stripifier::getNextIndex(scratchIndices, nextFace);

        auto nextNextFace = Stripifier::findOtherFace(edgeInfos, testnv0, testnv1, nextFace);

        if (!nextNextFace || isMarked(nextNextFace))
        {
            // Uh oh, we're following a dead end, try swapping
            auto testNextFace = Stripifier::findOtherFace(edgeInfos, nv0, testnv1, nextFace);
            if (testNextFace && !isMarked(testNextFace))
            {
                // We only swap if it buys us something

                // Add a "fake" degenerate face
                auto tempFace = new FaceInfo(nv0, nv1, nv0, true);

                backwardFaces.append(tempFace);
                markTriangle(tempFace);
                scratchIndices.append(nv0);
                testnv0 = nv0;

                degenerateCount++;
            }
        }

        // Add this to the strip
        backwardFaces.append(nextFace);

        tempAllFaces.append(nextFace);

        markTriangle(nextFace);

        // Add the index
        scratchIndices.append(testnv1);

        // And get the next face
        nv0 = testnv0;
        nv1 = testnv1;
        nextFace = Stripifier::findOtherFace(edgeInfos, nv0, nv1, nextFace);
    }

    // Combine the forward and backwards stripification lists and put into our own face vector
    backwardFaces.reverse();
    faces.append(backwardFaces);
    faces.append(forwardFaces);
}

// Returns true if the input face and the current strip share an edge
bool StripInfo::sharesEdge(const FaceInfo* faceInfo, Vector<EdgeInfo*>& edgeInfos)
{
    for (auto i = 0U; i < 3; i++)
    {
        auto currEdge = Stripifier::findEdgeInfo(edgeInfos, faceInfo->v[i], faceInfo->v[(i + 1) % 3]);
        if (hasFace(currEdge->face[0]) || hasFace(currEdge->face[1]))
            return true;
    }

    return false;
}

// Finds the next face to start the next strip on
bool Stripifier::findTraversal(Vector<FaceInfo*>& faceInfos, Vector<EdgeInfo*>& edgeInfos, StripInfo* strip,
                               StripStartInfo& startInfo)
{
    // If the strip was v[0]->v[1] on the edge, then v[1] will be a vertex in the next edge.
    auto v = strip->startInfo.toV1 ? strip->startInfo.startEdge->v[1] : strip->startInfo.startEdge->v[0];

    auto untouchedFace = pointer_to<FaceInfo>::type();

    auto edgeIter = edgeInfos[v];
    while (edgeIter)
    {
        if (edgeIter->face[0] && !strip->hasFace(edgeIter->face[0]) && edgeIter->face[1] && !strip->isMarked(edgeIter->face[1]))
        {
            untouchedFace = edgeIter->face[1];
            break;
        }

        if (edgeIter->face[1] && !strip->hasFace(edgeIter->face[1]) && edgeIter->face[0] && !strip->isMarked(edgeIter->face[0]))
        {
            untouchedFace = edgeIter->face[0];
            break;
        }

        // Find the next edgeIter
        edgeIter = (edgeIter->v[0] == v) ? edgeIter->nextV[0] : edgeIter->nextV[1];
    }

    startInfo.startFace = untouchedFace;
    startInfo.startEdge = edgeIter;
    if (edgeIter)
    {
        if (strip->sharesEdge(startInfo.startFace, edgeInfos))
            startInfo.toV1 = (edgeIter->v[0] == v);    // Note! used to be v1
        else
            startInfo.toV1 = (edgeIter->v[1] == v);
    }

    return startInfo.startFace != nullptr;
}

// Returns true if the next face should be ordered in clockwise fashion
bool Stripifier::isNextClockwise(unsigned int indexCount)
{
    return (indexCount % 2) == 0;
}

// Returns true if the face is ordered in clockwise fashion
bool Stripifier::isClockwise(const FaceInfo* faceInfo, unsigned int v0, unsigned int v1)
{
    if (faceInfo->v[0] == v0)
        return faceInfo->v[1] == v1;

    if (faceInfo->v[1] == v0)
        return faceInfo->v[2] == v1;

    return faceInfo->v[0] == v1;
}

// Generates actual strips from the list-in-strip-order
void Stripifier::createStrips(const Vector<StripInfo*>& allStrips, Vector<int>& stripIndices, bool stitchStrips,
                              unsigned int& stripCount)
{
    assert(stripCount == 0);

    auto lastFace = FaceInfo(0, 0, 0);

    // We infer the cw/ccw ordering depending on the number of indices. This is screwed up by the fact that we insert -1s to
    // denote changing strips, this is to account for that
    auto accountForNegatives = 0U;

    for (auto i = 0U; i < allStrips.size(); i++)
    {
        auto strip = allStrips[i];

        // Handle the first face in the strip
        {
            auto firstFace = FaceInfo(strip->faces[0]->v[0], strip->faces[0]->v[1], strip->faces[0]->v[2]);

            // If there is a second face, reorder vertices such that the unique vertex is first
            if (strip->faces.size() > 1)
            {
                auto unique = getUniqueVertexInB(strip->faces[1], &firstFace);
                if (unique == int(firstFace.v[1]))
                    std::swap(firstFace.v[0], firstFace.v[1]);
                else if (unique == int(firstFace.v[2]))
                    std::swap(firstFace.v[0], firstFace.v[2]);

                // If there is a third face, reorder vertices such that the shared vertex is last
                if (strip->faces.size() > 2)
                {
                    if (strip->faces[1]->isDegenerate())
                    {
                        auto pivot = strip->faces[1]->v[1];
                        if (firstFace.v[1] == pivot)
                            std::swap(firstFace.v[1], firstFace.v[2]);
                    }
                    else
                    {
                        auto shared0 = 0;
                        auto shared1 = 0;
                        getSharedVertices(strip->faces[2], &firstFace, shared0, shared1);
                        if (shared0 == int(firstFace.v[1]) && shared1 == -1)
                            std::swap(firstFace.v[1], firstFace.v[2]);
                    }
                }
            }

            if (i == 0 || !stitchStrips)
            {
                if (!isClockwise(strip->faces[0], firstFace.v[0], firstFace.v[1]))
                    stripIndices.append(firstFace.v[0]);
            }
            else
            {
                // Double tap the first in the new strip
                stripIndices.append(firstFace.v[0]);

                // Check ordering
                if (isNextClockwise(stripIndices.size() - accountForNegatives) !=
                    isClockwise(strip->faces[0], firstFace.v[0], firstFace.v[1]))
                    stripIndices.append(firstFace.v[0]);
            }

            stripIndices.append(firstFace.v[0]);
            stripIndices.append(firstFace.v[1]);
            stripIndices.append(firstFace.v[2]);

            // Update last face info
            lastFace = firstFace;
        }

        for (auto j = 1U; j < strip->faces.size(); j++)
        {
            auto unique = getUniqueVertexInB(&lastFace, strip->faces[j]);
            if (unique != -1)
            {
                stripIndices.append(unique);

                // Update last face info
                lastFace.v[0] = lastFace.v[1];
                lastFace.v[1] = lastFace.v[2];
                lastFace.v[2] = unique;
            }
            else
            {
                // We've hit a degenerate
                stripIndices.append(strip->faces[j]->v[2]);
                lastFace.v[0] = strip->faces[j]->v[0];
                lastFace.v[1] = strip->faces[j]->v[1];
                lastFace.v[2] = strip->faces[j]->v[2];
            }
        }

        // Double tap between strips.
        if (stitchStrips)
        {
            if (i != allStrips.size() - 1)
                stripIndices.append(lastFace.v[2]);
        }
        else
        {
            // -1 index indicates next strip
            stripIndices.append(-1);
            accountForNegatives++;
            stripCount++;
        }

        // Update last face info
        lastFace.v[0] = lastFace.v[1];
        lastFace.v[1] = lastFace.v[2];
    }

    if (stitchStrips)
        stripCount = 1;
}

bool Stripifier::stripify(const Vector<unsigned int>& indices, unsigned int maxIndex, Vector<StripInfo*>& outStrips,
                          Vector<FaceInfo*>& outFaceList, Runnable& r)
{
    indices_ = indices;
    meshJump_ = 0.0f;
    firstTimeResetPoint_ = true;

    // Build the stripification info
    r.beginTask("", 5);
    auto allFaceInfos = Vector<FaceInfo*>();
    auto allEdgeInfos = Vector<EdgeInfo*>();
    if (!buildstripifyInfo(allFaceInfos, allEdgeInfos, maxIndex, r))
        return false;
    r.endTask();

    auto allStrips = Vector<StripInfo*>();

    // Stripify
    r.beginTask("", 45);
    if (!findAllStrips(allStrips, allFaceInfos, allEdgeInfos, 10, r))
        return false;
    r.endTask();

    // Split up the strips into cache friendly pieces, optimize them, then put into outStrips
    r.beginTask("", 50);
    if (!splitUpStripsAndOptimize(allStrips, outStrips, allEdgeInfos, outFaceList, r))
        return false;
    r.endTask();

    // Clean up
    for (auto strip : allStrips)
        delete strip;

    for (auto i = 0U; i < allEdgeInfos.size(); i++)
    {
        auto info = allEdgeInfos[i];
        while (info)
        {
            auto next = (info->v[0] == i) ? info->nextV[0] : info->nextV[1];
            info->release();
            info = next;
        }
    }

    return true;
}

// Splits the input vector of strips into smaller, cache friendly pieces, then reorders these pieces to maximize cache hits. The
// final strips are output through outStrips.
bool Stripifier::splitUpStripsAndOptimize(Vector<StripInfo*>& allStrips, Vector<StripInfo*>& outStrips,
                                          Vector<EdgeInfo*>& edgeInfos, Vector<FaceInfo*>& outFaceList, Runnable& r)
{
    outStrips.clear();

    auto tempStrips = Vector<StripInfo*>();

    // Split up strips into threshold-sized pieces
    for (auto strip : allStrips)
    {
        auto currentStrip = pointer_to<StripInfo>::type();
        auto startInfo = StripStartInfo(nullptr, nullptr, false);

        auto actualStripSize = 0U;
        for (auto face : strip->faces)
        {
            if (!face->isDegenerate())
                actualStripSize++;
        }

        if (actualStripSize > CacheSize)
        {
            auto numTimes = actualStripSize / CacheSize;
            auto numLeftover = actualStripSize % CacheSize;

            auto degenerateCount = 0U;
            auto j = 0U;
            for (; j < numTimes; j++)
            {
                currentStrip = new StripInfo(startInfo, -1);

                auto faceCtr = j * CacheSize + degenerateCount;
                auto firstTime = true;
                while (faceCtr < CacheSize + (j * CacheSize) + degenerateCount)
                {
                    if (strip->faces[faceCtr]->isDegenerate())
                    {
                        degenerateCount++;

                        // Last time or first time through, no need for a degenerate
                        if ((((faceCtr + 1) != CacheSize + (j * CacheSize) + degenerateCount) ||
                             ((j == numTimes - 1) && (numLeftover < 4) && (numLeftover > 0))) &&
                            !firstTime)
                            currentStrip->faces.append(strip->faces[faceCtr++]);
                        else
                        {
                            // But we do need to delete the degenerate, if it's marked fake, to avoid leaking
                            if (strip->faces[faceCtr]->isFake)
                            {
                                delete strip->faces[faceCtr];
                                strip->faces[faceCtr] = nullptr;
                            }
                            faceCtr++;
                        }
                    }
                    else
                    {
                        currentStrip->faces.append(strip->faces[faceCtr++]);
                        firstTime = false;
                    }
                }

                if (j == numTimes - 1)    // Last time through
                {
                    if (numLeftover < 4 && numLeftover > 0)    // Way too small
                    {
                        // Just add to last strip
                        auto ctr = 0U;
                        while (ctr < numLeftover)
                        {
                            strip->faces[faceCtr]->isDegenerate() ? degenerateCount++ : ctr++;
                            currentStrip->faces.append(strip->faces[faceCtr++]);
                        }
                        numLeftover = 0;
                    }
                }
                tempStrips.append(currentStrip);
            }

            auto leftOff = j * CacheSize + degenerateCount;

            if (numLeftover)
            {
                currentStrip = new StripInfo(startInfo, -1);

                auto ctr = 0U;
                auto firstTime = true;
                while (ctr < numLeftover)
                {
                    if (!strip->faces[leftOff]->isDegenerate())
                    {
                        ctr++;
                        firstTime = false;
                        currentStrip->faces.append(strip->faces[leftOff++]);
                    }
                    else if (!firstTime)
                        currentStrip->faces.append(strip->faces[leftOff++]);
                    else
                    {
                        if (strip->faces[leftOff]->isFake)
                        {
                            delete strip->faces[leftOff];
                            strip->faces[leftOff] = nullptr;
                        }

                        leftOff++;
                    }
                }

                tempStrips.append(currentStrip);
            }
        }
        else
        {
            // We're not just doing a tempStrips.append(allBigStrips[i]) because this way we can delete allBigStrips later to
            // free the memory
            currentStrip = new StripInfo(startInfo, -1);
            currentStrip->faces.append(strip->faces);

            tempStrips.append(currentStrip);
        }
    }

    if (tempStrips.size())
    {
        // Optimize for the vertex cache
        auto vertexCache = VertexCache(CacheSize);

        auto firstIndex = 0;
        auto minCost = 10000.0f;

        for (auto i = 0U; i < tempStrips.size(); i++)
        {
            auto neighborCount = 0U;

            // Find strip with least number of neighbors per face
            for (auto face : tempStrips[i]->faces)
            {
                if (findOtherFace(edgeInfos, face->v[0], face->v[1], face))
                    neighborCount++;
                if (findOtherFace(edgeInfos, face->v[1], face->v[2], face))
                    neighborCount++;
                if (findOtherFace(edgeInfos, face->v[2], face->v[0], face))
                    neighborCount++;
            }

            auto currCost = float(neighborCount) / float(tempStrips[i]->faces.size());
            if (currCost < minCost)
            {
                minCost = currCost;
                firstIndex = i;
            }
        }

        updateCacheStrip(vertexCache, tempStrips[firstIndex]);
        outStrips.append(tempStrips[firstIndex]);

        tempStrips[firstIndex]->visited = true;

        auto wantsClockwise = (tempStrips[firstIndex]->faces.size() % 2) == 0;

        auto numStripsDone = 0U;

        // This O(N ^ 2) algorithm is what slows down stripification so much, needs to be improved
        while (true)
        {
            auto bestNumHits = -1.0f;
            auto bestIndex = 0U;

            // Find best strip to add next, given the current cache
            for (auto i = 0U; i < tempStrips.size(); i++)
            {
                if (tempStrips[i]->visited)
                    continue;

                auto cacheHitCount = 0U;
                for (auto face : tempStrips[i]->faces)
                {
                    if (vertexCache.has(face->v[0]))
                        cacheHitCount++;
                    if (vertexCache.has(face->v[1]))
                        cacheHitCount++;
                    if (vertexCache.has(face->v[2]))
                        cacheHitCount++;
                }

                auto numHits = float(cacheHitCount) / float(tempStrips[i]->faces.size());
                if (numHits > bestNumHits)
                {
                    bestNumHits = numHits;
                    bestIndex = i;
                }
                else if (numHits >= bestNumHits)
                {
                    // Check previous strip to see if this one requires it to switch polarity
                    auto strip = tempStrips[i];
                    auto stripFaceCount = strip->faces.size();

                    auto firstFace = FaceInfo(strip->faces[0]->v[0], strip->faces[0]->v[1], strip->faces[0]->v[2]);

                    // If there is a second face, reorder vertices such that the unique vertex is first
                    if (stripFaceCount > 1)
                    {
                        auto unique = Stripifier::getUniqueVertexInB(strip->faces[1], &firstFace);
                        if (unique == int(firstFace.v[1]))
                            std::swap(firstFace.v[0], firstFace.v[1]);
                        else if (unique == int(firstFace.v[2]))
                            std::swap(firstFace.v[0], firstFace.v[2]);

                        // If there is a third face, reorder vertices such that the shared vertex is last
                        if (stripFaceCount > 2)
                        {
                            auto shared0 = 0;
                            auto shared1 = 0;
                            getSharedVertices(strip->faces[2], &firstFace, shared0, shared1);
                            if (shared0 == int(firstFace.v[1]) && shared1 == -1)
                                std::swap(firstFace.v[1], firstFace.v[2]);
                        }
                    }

                    // Check ordering
                    if (wantsClockwise == isClockwise(strip->faces[0], firstFace.v[0], firstFace.v[1]))
                        bestIndex = i;
                }
            }

            if (bestNumHits == -1.0f)
                break;

            tempStrips[bestIndex]->visited = true;
            updateCacheStrip(vertexCache, tempStrips[bestIndex]);
            outStrips.append(tempStrips[bestIndex]);
            wantsClockwise = (tempStrips[bestIndex]->faces.size() % 2 == 0) ? wantsClockwise : !wantsClockwise;

            // Check in with runnable
            if (r.setTaskProgress(++numStripsDone + 1, tempStrips.size()))
                return false;
        }
    }

    return true;
}

// Updates the input vertex cache with this strip's vertices
void Stripifier::updateCacheStrip(VertexCache& vertexCache, const StripInfo* strip)
{
    for (auto face : strip->faces)
    {
        vertexCache.add(face->v[0]);
        vertexCache.add(face->v[1]);
        vertexCache.add(face->v[2]);
    }
}

// Does the stripification, puts output strips into allStrips. Works by setting running a number of experiments in different
// areas of the mesh, and accepting the one which results in the longest strips. It then accepts this, and moves on to a
// different area of the mesh. We try to jump around the mesh some, to ensure that large open spans of strips get generated.
bool Stripifier::findAllStrips(Vector<StripInfo*>& allStrips, Vector<FaceInfo*>& allFaceInfos, Vector<EdgeInfo*>& allEdgeInfos,
                               unsigned int sampleCount, Runnable& r)
{
    auto experimentID = 0;
    auto done = false;

    while (!done)
    {
        // Phase 1: Set up sampleCount * numEdges experiments
        auto experiments = Vector<Vector<StripInfo*>>();
        auto resetPoints = std::set<FaceInfo*>();
        for (auto i = 0U; i < sampleCount; i++)
        {
            // Try to find another good reset point. If there are none to be found, we are done.
            auto nextFace = findGoodResetPoint(allFaceInfos, allEdgeInfos);
            if (!nextFace)
            {
                done = true;
                break;
            }
            else if (resetPoints.find(nextFace) != resetPoints.end())
                continue;    // Already evaluated starting at this face in this slew of experiments, skip going any further

            assert(!nextFace->strip);

            // Trying it now ...
            resetPoints.insert(nextFace);

            // Otherwise now try experiments for starting on the 01, 12, and 20 edges
            for (auto j = 0U; j < 3; j++)
            {
                auto edge = findEdgeInfo(allEdgeInfos, nextFace->v[j], nextFace->v[(j + 1) % 3]);

                experiments.emplace(1, new StripInfo(StripStartInfo(nextFace, edge, true), experimentID++));
                experiments.emplace(1, new StripInfo(StripStartInfo(nextFace, edge, false), experimentID++));
            }
        }

        if (done)
            break;

        // Phase 2: Iterate through that we setup in the last phase and really build each of the strips and strips that follow
        // to see how far we get
        for (auto& experiment : experiments)
        {
            // Build the first strip of the list
            experiment[0]->build(allEdgeInfos, allFaceInfos);
            experimentID = experiment[0]->experimentID;

            auto stripIter = experiment[0];
            auto startInfo = StripStartInfo(nullptr, nullptr, false);
            while (findTraversal(allFaceInfos, allEdgeInfos, stripIter, startInfo))
            {
                // Create the new strip info
                stripIter = new StripInfo(startInfo, experimentID);

                // Build the next strip
                stripIter->build(allEdgeInfos, allFaceInfos);

                // Add it to the list
                experiment.append(stripIter);
            }
        }

        // Phase 3: Find the experiment that has the most promise
        auto bestIndex = 0U;
        auto bestValue = 0.0f;
        for (auto i = 0U; i < experiments.size(); i++)
        {
            auto faceCount = 0U;

            for (auto stripInfo : experiments[i])
            {
                faceCount += stripInfo->faces.size();
                faceCount -= stripInfo->degenerateCount;
            }

            auto value = float(faceCount) / float(experiments[i].size());
            if (value > bestValue)
            {
                bestValue = value;
                bestIndex = i;
            }
        }

        // Phase 4: commit the best experiment of the bunch by setting their experimentID to -1 and adding to the allStrips
        // vector
        for (auto strip : experiments[bestIndex])
        {
            // Tell the strip that it is now real
            strip->experimentID = -1;

            // Add to the list of real strips
            allStrips.append(strip);

            // Iterate through the faces of the strip, tell the faces of the strip that they belong to a real strip now
            for (auto face : strip->faces)
                strip->markTriangle(face);
        }
        experiments.erase(bestIndex);

        // And destroy all of the others
        for (auto& experiment : experiments)
        {
            for (auto& currStrip : experiment)
            {
                // Delete all bogus faces in the experiments
                for (auto face : currStrip->faces)
                {
                    if (face->isFake)
                        delete face;
                }

                delete currStrip;
                currStrip = nullptr;
            }
        }

        // See how many tris have been put into strips
        auto completed = 0U;
        for (auto& faceInfo : allFaceInfos)
        {
            if (faceInfo->strip)
                completed++;
        }
        if (r.setTaskProgress(completed, allFaceInfos.size()))
            return false;
    }

    return true;
}

bool TriangleStripper::run(const Vector<unsigned int>& indices, Vector<PrimitiveWithIndices>& result, Runnable& r)
{
    if (indices.empty())
        return true;

    auto maxIndex = 0U;
    auto minIndex = 0U;
    Math::calculateBounds(indices.getData(), indices.size(), minIndex, maxIndex);

    auto stripifier = Stripifier();
    auto strips = Vector<StripInfo*>();
    auto faces = Vector<FaceInfo*>();

    // Do actual stripification
    if (!stripifier.stripify(indices, maxIndex, strips, faces, r))
        return false;

    // Stitch strips together
    auto stripIndices = Vector<int>();

    auto stripCount = 0U;
    stripifier.createStrips(strips, stripIndices, true, stripCount);

    // Convert to output format, adding one extra entry if there is a triangle list
    result.resize(stripCount + faces.size() ? 1 : 0);

    // First, the strips
    auto offset = 0U;
    for (auto j = 0U; j < stripCount; j++)
    {
        // If we've got multiple strips, we need to figure out the correct length
        auto i = offset;
        for (; i < stripIndices.size(); i++)
        {
            if (stripIndices[i] == -1)    // Index of -1 indicates the start of a new strip
                break;
        }
        auto stripLength = i - offset;

        result[j].first = GraphicsInterface::TriangleStrip;
        result[j].second.resize(stripLength);
        for (i = 0; i < stripLength; i++)
            result[j].second[i] = stripIndices[offset + i];

        // Add 1 to account for the -1 separating strips. This doesn't break the stitched case since we'll exit the loop.
        offset += stripLength + 1;
    }

    // Next, the list, if any
    if (faces.size())
    {
        result.back().first = GraphicsInterface::TriangleList;
        result.back().second.reserve(faces.size() * 3);
        for (auto& face : faces)
        {
            result.back().second.append(face->v[0]);
            result.back().second.append(face->v[1]);
            result.back().second.append(face->v[2]);
        }
    }

    // Clean up everything
    for (auto strip : strips)
    {
        for (auto face : strip->faces)
        {
            delete face;
            face = nullptr;
        }

        delete strip;
        strip = nullptr;
    }

    for (auto face : faces)
    {
        delete face;
        face = nullptr;
    }

    return true;
}

}
