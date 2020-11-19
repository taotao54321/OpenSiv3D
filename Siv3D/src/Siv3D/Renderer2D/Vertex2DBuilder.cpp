﻿//-----------------------------------------------
//
//	This file is part of the Siv3D Engine.
//
//	Copyright (c) 2008-2020 Ryo Suzuki
//	Copyright (c) 2016-2020 OpenSiv3D Project
//
//	Licensed under the MIT License.
//
//-----------------------------------------------

# include "Vertex2DBuilder.hpp"
# include <Siv3D/FastMath.hpp>

namespace s3d
{
	namespace detail
	{
		static constexpr Vertex2D::IndexType RectIndexTable[6] = { 0, 1, 2, 2, 1, 3 };

		static constexpr Vertex2D::IndexType RectFrameIndexTable[24] = { 0, 1, 2, 3, 2, 1, 0, 4, 1, 5, 1, 4, 5, 4, 7, 6, 7, 4, 3, 7, 2, 6, 2, 7 };

		static constexpr Vertex2D::IndexType MaxSinCosTableQuality = 40;

		static constexpr Vertex2D::IndexType SinCosTableSize = ((MaxSinCosTableQuality - 5) * (6 + (MaxSinCosTableQuality))) / 2;

		static const std::array<Float2, SinCosTableSize> CircleSinCosTable = []()
		{
			std::array<Float2, SinCosTableSize> results;
			Float2* pDst = results.data();

			for (int32 quality = 6; quality <= MaxSinCosTableQuality; ++quality)
			{
				const float radDelta = Math::Constants::TwoPiF / quality;

				for (Vertex2D::IndexType i = 0; i < quality; ++i)
				{
					const float rad = radDelta * i;
					(pDst++)->set(std::cos(rad), -std::sin(rad));
				}
			}

			return results;
		}();

		[[nodiscard]]
		inline const Float2* GetSinCosTableStartPtr(const uint16 quality) noexcept
		{
			return &CircleSinCosTable[((quality - 6) * (6 + (quality - 1))) / 2];
		}

		[[nodiscard]]
		inline constexpr Vertex2D::IndexType CalculateCircleQuality(const float size) noexcept
		{
			if (size <= 5.0f)
			{
				return static_cast<Vertex2D::IndexType>(size + 3) * 2;
			}
			else
			{
				return static_cast<Vertex2D::IndexType>(Min(18 + (size - 5.0f) / 2.2f, 255.0f));
			}
		}

		[[nodiscard]]
		inline constexpr Vertex2D::IndexType CalculateCircleFrameQuality(const float size) noexcept
		{
			if (size <= 1.0f)
			{
				return 6;
			}
			else if (size <= 8.0f)
			{
				return Max(static_cast<Vertex2D::IndexType>(2.0 * size), Vertex2D::IndexType(8));
			}
			else
			{
				return static_cast<Vertex2D::IndexType>(Min(16 + (size - 8.0f) / 2.2f, 255.0f));
			}
		}
	}

	namespace Vertex2DBuilder
	{
		Vertex2D::IndexType BuildDefaultLine(const BufferCreatorFunc& bufferCreator, const Float2& begin, const Float2& end, const float thickness, const Float4(&colors)[2])
		{
			if (thickness <= 0.0f)
			{
				return 0;
			}

			constexpr Vertex2D::IndexType vertexSize = 4, indexSize = 6;
			auto [pVertex, pIndex, indexOffset] = bufferCreator(vertexSize, indexSize);

			if (not pVertex)
			{
				return 0;
			}

			const float halfThickness = (thickness * 0.5f);
			const Float2 line = (end - begin).normalized();
			const Float2 vNormal{ (-line.y * halfThickness), (line.x * halfThickness) };
			const Float2 lineHalf{ line * halfThickness };

			const Float2 begin2 = (begin - lineHalf);
			const Float2 end2 = (end + lineHalf);

			pVertex[0].set(begin2 + vNormal , colors[0]);
			pVertex[1].set(begin2 - vNormal, colors[0]);
			pVertex[2].set(end2 + vNormal, colors[1]);
			pVertex[3].set(end2 - vNormal, colors[1]);

			for (Vertex2D::IndexType i = 0; i < indexSize; ++i)
			{
				*pIndex++ = indexOffset + detail::RectIndexTable[i];
			}

			return indexSize;
		}

		Vertex2D::IndexType BuildTriangle(const BufferCreatorFunc& bufferCreator, const Float2(&points)[3], const Float4& color)
		{
			constexpr Vertex2D::IndexType vertexSize = 3, indexSize = 3;
			auto [pVertex, pIndex, indexOffset] = bufferCreator(vertexSize, indexSize);

			if (not pVertex)
			{
				return 0;
			}

			pVertex[0].set(points[0], color);
			pVertex[1].set(points[1], color);
			pVertex[2].set(points[2], color);

			pIndex[0] = indexOffset;
			pIndex[1] = (indexOffset + 1);
			pIndex[2] = (indexOffset + 2);

			return indexSize;
		}

		Vertex2D::IndexType BuildTriangle(const BufferCreatorFunc& bufferCreator, const Float2(&points)[3], const Float4(&colors)[3])
		{
			constexpr Vertex2D::IndexType vertexSize = 3, indexSize = 3;
			auto [pVertex, pIndex, indexOffset] = bufferCreator(vertexSize, indexSize);

			if (not pVertex)
			{
				return 0;
			}

			pVertex[0].set(points[0], colors[0]);
			pVertex[1].set(points[1], colors[1]);
			pVertex[2].set(points[2], colors[2]);

			pIndex[0] = indexOffset;
			pIndex[1] = (indexOffset + 1);
			pIndex[2] = (indexOffset + 2);

			return indexSize;
		}

		Vertex2D::IndexType BuildRect(const BufferCreatorFunc& bufferCreator, const FloatRect& rect, const Float4& color)
		{
			constexpr Vertex2D::IndexType vertexSize = 4, indexSize = 6;
			auto [pVertex, pIndex, indexOffset] = bufferCreator(vertexSize, indexSize);

			if (not pVertex)
			{
				return 0;
			}

			pVertex[0].set(rect.left, rect.top, color);
			pVertex[1].set(rect.right, rect.top, color);
			pVertex[2].set(rect.left, rect.bottom, color);
			pVertex[3].set(rect.right, rect.bottom, color);

			for (Vertex2D::IndexType i = 0; i < indexSize; ++i)
			{
				*pIndex++ = (indexOffset + detail::RectIndexTable[i]);
			}

			return indexSize;
		}

		Vertex2D::IndexType BuildRect(const BufferCreatorFunc& bufferCreator, const FloatRect& rect, const Float4(&colors)[4])
		{
			constexpr Vertex2D::IndexType vertexSize = 4, indexSize = 6;
			auto [pVertex, pIndex, indexOffset] = bufferCreator(vertexSize, indexSize);

			if (not pVertex)
			{
				return 0;
			}

			pVertex[0].set(rect.left, rect.top, colors[0]);
			pVertex[1].set(rect.right, rect.top, colors[1]);
			pVertex[2].set(rect.left, rect.bottom, colors[3]);
			pVertex[3].set(rect.right, rect.bottom, colors[2]);

			for (Vertex2D::IndexType i = 0; i < indexSize; ++i)
			{
				*pIndex++ = (indexOffset + detail::RectIndexTable[i]);
			}

			return indexSize;
		}

		Vertex2D::IndexType BuildRectFrame(const BufferCreatorFunc& bufferCreator, const FloatRect& rect, float thickness, const Float4& innerColor, const Float4& outerColor)
		{
			constexpr Vertex2D::IndexType vertexSize = 8, indexSize = 24;
			auto [pVertex, pIndex, indexOffset] = bufferCreator(vertexSize, indexSize);

			if (not pVertex)
			{
				return 0;
			}

			pVertex[0].pos.set((rect.left - thickness), (rect.top - thickness));
			pVertex[1].pos.set(rect.left, rect.top);
			pVertex[2].pos.set((rect.left - thickness), (rect.bottom + thickness));
			pVertex[3].pos.set(rect.left, rect.bottom);
			pVertex[4].pos.set((rect.right + thickness), (rect.top - thickness));
			pVertex[5].pos.set(rect.right, rect.top);
			pVertex[6].pos.set((rect.right + thickness), (rect.bottom + thickness));
			pVertex[7].pos.set(rect.right, rect.bottom);

			for (size_t i = 0; i < 4; ++i)
			{
				(pVertex++)->color = outerColor;
				(pVertex++)->color = innerColor;
			}

			for (Vertex2D::IndexType i = 0; i < indexSize; ++i)
			{
				*pIndex++ = (indexOffset + detail::RectFrameIndexTable[i]);
			}

			return indexSize;
		}

		Vertex2D::IndexType BuildCircle(const BufferCreatorFunc& bufferCreator, const Float2& center, float r, const Float4& innerColor, const Float4& outerColor, const float scale)
		{
			const float absR = Abs(r);
			const Vertex2D::IndexType quality = detail::CalculateCircleQuality(absR * scale);
			const Vertex2D::IndexType vertexSize = (quality + 1), indexSize = (quality * 3);
			auto [pVertex, pIndex, indexOffset] = bufferCreator(vertexSize, indexSize);

			if (not pVertex)
			{
				return 0;
			}

			// 中心
			const float centerX = center.x;
			const float centerY = center.y;
			pVertex[0].pos.set(centerX, centerY);

			// 周
			if (quality <= detail::MaxSinCosTableQuality)
			{
				const Float2* pCS = detail::GetSinCosTableStartPtr(quality);
				Vertex2D* pDst = &pVertex[1];

				for (Vertex2D::IndexType i = 0; i < quality; ++i)
				{
					(pDst++)->pos.set(r * pCS->x + centerX, r * pCS->y + centerY);
					++pCS;
				}
			}
			else
			{
				const float radDelta = Math::TwoPiF / quality;
				Vertex2D* pDst = &pVertex[1];

				for (Vertex2D::IndexType i = 0; i < quality; ++i)
				{
					const float rad = (radDelta * i);
					const auto [s, c] = FastMath::SinCos(rad);
					(pDst++)->pos.set(centerX + r * c, centerY - r * s);
				}
			}

			{
				(pVertex++)->color = innerColor;

				for (size_t i = 1; i < vertexSize; ++i)
				{
					(pVertex++)->color = outerColor;
				}
			}

			{
				for (Vertex2D::IndexType i = 0; i < (quality - 1); ++i)
				{
					*pIndex++ = indexOffset + (i + 1);
					*pIndex++ = indexOffset;
					*pIndex++ = indexOffset + (i + 2);
				}

				*pIndex++ = (indexOffset + quality);
				*pIndex++ = indexOffset;
				*pIndex++ = (indexOffset + 1);
			}

			return indexSize;
		}

		Vertex2D::IndexType BuildCircleFrame(const BufferCreatorFunc& bufferCreator, const Float2& center, const float rInner, const float thickness, const Float4& innerColor, const Float4& outerColor, const float scale)
		{
			const float rOuter = (rInner + thickness);
			const Vertex2D::IndexType quality = detail::CalculateCircleFrameQuality(rOuter * scale);
			const Vertex2D::IndexType vertexSize = quality * 2, indexSize = quality * 6;
			auto [pVertex, pIndex, indexOffset] = bufferCreator(vertexSize, indexSize);

			if (not pVertex)
			{
				return 0;
			}

			const float centerX = center.x;
			const float centerY = center.y;

			if (quality <= detail::MaxSinCosTableQuality)
			{
				const Float2* pCS = detail::GetSinCosTableStartPtr(quality);
				Vertex2D* pDst = pVertex;

				for (Vertex2D::IndexType i = 0; i < quality; ++i)
				{
					(pDst++)->pos.set(rOuter * pCS->x + centerX, rOuter * pCS->y + centerY);
					(pDst++)->pos.set(rInner * pCS->x + centerX, rInner * pCS->y + centerY);
					++pCS;
				}
			}
			else
			{
				const float radDelta = Math::TwoPiF / quality;
				Vertex2D* pDst = pVertex;

				for (Vertex2D::IndexType i = 0; i < quality; ++i)
				{
					const float rad = (radDelta * i);
					const auto [s, c] = FastMath::SinCos(rad);
					(pDst++)->pos.set(centerX + rOuter * c, centerY - rOuter * s);
					(pDst++)->pos.set(centerX + rInner * c, centerY - rInner * s);
				}
			}

			for (Vertex2D::IndexType i = 0; i < quality; ++i)
			{
				(pVertex++)->color = outerColor;
				(pVertex++)->color = innerColor;
			}

			for (Vertex2D::IndexType i = 0; i < quality; ++i)
			{
				for (Vertex2D::IndexType k = 0; k < 6; ++k)
				{
					*pIndex++ = (indexOffset + (i * 2 + detail::RectIndexTable[k]) % (quality * 2));
				}
			}

			return indexSize;
		}

		Vertex2D::IndexType BuildQuad(const BufferCreatorFunc& bufferCreator, const FloatQuad& quad, const Float4 color)
		{
			constexpr Vertex2D::IndexType vertexSize = 4, indexSize = 6;
			auto [pVertex, pIndex, indexOffset] = bufferCreator(vertexSize, indexSize);

			if (not pVertex)
			{
				return 0;
			}

			pVertex[0].set(quad.p[0], color);
			pVertex[1].set(quad.p[1], color);
			pVertex[2].set(quad.p[3], color);
			pVertex[3].set(quad.p[2], color);

			for (Vertex2D::IndexType i = 0; i < indexSize; ++i)
			{
				*pIndex++ = (indexOffset + detail::RectIndexTable[i]);
			}

			return indexSize;
		}

		Vertex2D::IndexType BuildQuad(const BufferCreatorFunc& bufferCreator, const FloatQuad& quad, const Float4(&colors)[4])
		{
			constexpr Vertex2D::IndexType vertexSize = 4, indexSize = 6;
			auto [pVertex, pIndex, indexOffset] = bufferCreator(vertexSize, indexSize);

			if (not pVertex)
			{
				return 0;
			}

			pVertex[0].set(quad.p[0], colors[0]);
			pVertex[1].set(quad.p[1], colors[1]);
			pVertex[2].set(quad.p[3], colors[3]);
			pVertex[3].set(quad.p[2], colors[2]);

			for (Vertex2D::IndexType i = 0; i < indexSize; ++i)
			{
				*pIndex++ = (indexOffset + detail::RectIndexTable[i]);
			}

			return indexSize;
		}

		Vertex2D::IndexType BuildDefaultLineString(const BufferCreatorFunc& bufferCreator, const Vec2* points, const size_t size, const Optional<Float2>& offset, const float thickness, const bool inner, const Float4& color, const IsClosed isClosed, const float scale)
		{
			if ((size < 2)
				|| (32760 <= size)
				|| (thickness <= 0.0f)
				|| (not points))
			{
				return 0;
			}

			const float th2 = (0.01f / scale);
			const double th2D = th2;

			Array<Float2> buf(Arg::reserve = size);
			{
				buf.push_back(points[0]);

				for (size_t i = 1; i < (size - 1); ++i)
				{
					const Vec2 back = points[i - 1];
					const Vec2 current = points[i];

					if (back.distanceFromSq(current) < th2D)
					{
						continue;
					}

					buf.push_back(current);
				}

				const Vec2 back = points[size - 2];
				const Vec2 current = points[size - 1];

				if (back.distanceFromSq(current) >= th2D)
				{
					buf.push_back(current);
				}

				if (isClosed
					&& (buf.size() >= 2)
					&& buf.back().distanceFromSq(buf.front()) <= th2)
				{
					buf.pop_back();
				}

				if (buf.size() < 2)
				{
					return 0;
				}
			}

			const float threshold = 0.55f;

			Array<Float2> buf2;
			{
				buf2.push_back(buf.front());

				const size_t count = (buf.size() - 1 + static_cast<bool>(isClosed));

				for (size_t i = 1; i < count; ++i)
				{
					const Float2 back = buf[i - 1];
					const Float2 current = buf[i];
					const Float2 next = buf[(i + 1) % buf.size()];

					const Float2 v1 = (back - current).normalized();
					const Float2 v2 = (next - current).normalized();

					buf2.push_back(current);

					if ((not inner)
						&& (v1.dot(v2) > threshold))
					{
						const Float2 line = (current - back);
						const Float2 normal = Float2{ -line.y, line.x }.normalized();
						const Float2 tangent = ((next - current).normalized() + (current - back).normalized()).normalized();
						const Float2 line2 = (next - current);

						if (tangent.dot(line2) >= (-tangent).dot(line2))
						{
							buf2.push_back(current + tangent.normalized() * th2);
						}
						else if (tangent.dot(line2) <= (-tangent).dot(line2))
						{
							buf2.push_back(current + (-tangent).normalized() * th2);
						}
						else
						{
							buf2.push_back(current + normal * 0.001f);
						}
					}
				}

				if (isClosed)
				{
					const Float2 back = buf[buf.size() - 1];
					const Float2 current = buf[0];
					const Float2 next = buf[1];

					const Float2 v1 = (back - current).normalized();
					const Float2 v2 = (next - current).normalized();

					if ((not inner)
						&& (v1.dot(v2) > threshold))
					{
						const Float2 line = (current - back);
						const Float2 normal = Float2{ -line.y, line.x }.normalized();
						const Float2 tangent = ((next - current).normalized() + (current - back).normalized()).normalized();
						const Float2 line2 = (next - current);

						if (tangent.dot(line2) >= (-tangent).dot(line2))
						{
							buf2.push_back(current - tangent.normalized() * th2);
						}
						else if (tangent.dot(line2) <= (-tangent).dot(line2))
						{
							buf2.push_back(current - (-tangent).normalized() * th2);
						}
						else
						{
							buf2.push_back(current - normal * 0.001f);
						}
					}
				}
				else
				{
					buf2.push_back(buf.back());
				}
			}

			const Vertex2D::IndexType newSize = static_cast<Vertex2D::IndexType>(buf2.size());

			const Vertex2D::IndexType vertexSize = (newSize * 2), indexSize = (6 * (newSize - 1) + (static_cast<bool>(isClosed) * 6));
			auto [pVertex, pIndex, indexOffset] = bufferCreator(vertexSize, indexSize);

			if (not pVertex)
			{
				return 0;
			}

			const float thicknessHalf = (thickness * 0.5f);
			Float2 vNormalBegin{ 0, 0 }, vNormalEnd{ 0, 0 };
			const bool hasCap = (not isClosed);

			if (isClosed)
			{
				const Float2 p0 = buf2.back();
				const Float2 p1 = buf2[0];
				const Float2 p2 = buf2[1];
				const Float2 line = p1 - p0;
				const Float2 normal = Float2{ -line.y, line.x }.normalized();
				const Float2 tangent = ((p2 - p1).normalized() + (p1 - p0).normalized()).normalized();
				const Float2 miter = Float2{ -tangent.y, tangent.x };
				const float length = thicknessHalf / miter.dot(normal);
				const Float2 result0 = p1 + miter * length;
				const Float2 result1 = p1 - miter * length;

				pVertex[0].pos.set(result0);
				pVertex[1].pos.set(result1);
			}
			else
			{
				const Float2 p0 = buf2[0];
				const Float2 p1 = buf2[1];
				const Float2 line = (p1 - p0).normalize();
				vNormalBegin = Float2(-line.y * thicknessHalf, line.x * thicknessHalf);
				const Float2 lineHalf(line * thicknessHalf);

				pVertex[0].pos.set(p0 + vNormalBegin - lineHalf * hasCap);
				pVertex[1].pos.set(p0 - vNormalBegin - lineHalf * hasCap);
			}

			for (Vertex2D::IndexType i = 0; i < (size - 2); ++i)
			{
				const Float2 p0 = buf2[i];
				const Float2 p1 = buf2[i + 1];
				const Float2 p2 = buf2[i + 2];
				const Float2 line = p1 - p0;
				const Float2 normal = Float2{ -line.y, line.x }.normalized();
				const Float2 tangent = ((p2 - p1).normalized() + (p1 - p0).normalized()).normalized();
				const Float2 miter = Float2{ -tangent.y, tangent.x };
				const float length = thicknessHalf / miter.dot(normal);
				const Float2 result0 = p1 + miter * length;
				const Float2 result1 = p1 - miter * length;

				pVertex[i * 2 + 2].pos.set(result0);
				pVertex[i * 2 + 3].pos.set(result1);
			}

			if (isClosed)
			{
				const Float2 p0 = buf2[size - 2];
				const Float2 p1 = buf2[size - 1];
				const Float2 p2 = buf2[0];
				const Float2 line = p1 - p0;
				const Float2 normal = Float2{ -line.y, line.x }.normalized();
				const Float2 tangent = ((p2 - p1).normalized() + (p1 - p0).normalized()).normalized();
				const Float2 miter = Float2{ -tangent.y, tangent.x };
				const float length = thicknessHalf / miter.dot(normal);
				const Float2 result0 = p1 + miter * length;
				const Float2 result1 = p1 - miter * length;

				pVertex[size * 2 - 2].pos.set(result0);
				pVertex[size * 2 - 1].pos.set(result1);
			}
			else
			{
				const Float2 p0 = buf2[size - 2];
				const Float2 p1 = buf2[size - 1];
				const Float2 line = (p1 - p0).normalize();
				vNormalEnd = Float2(-line.y * thicknessHalf, line.x * thicknessHalf);
				const Float2 lineHalf(line * thicknessHalf);

				pVertex[size * 2 - 2].pos.set(p1 + vNormalEnd + lineHalf * hasCap);
				pVertex[size * 2 - 1].pos.set(p1 - vNormalEnd + lineHalf * hasCap);
			}

			if (offset)
			{
				const Float2 v = offset.value();
				Vertex2D* pDst = pVertex;

				for (Vertex2D::IndexType i = 0; i < vertexSize; ++i)
				{
					(pDst++)->pos.moveBy(v);
				}
			}

			for (size_t i = 0; i < vertexSize; ++i)
			{
				(pVertex++)->color = color;
			}

			{
				const Vertex2D::IndexType count = static_cast<Vertex2D::IndexType>(size - 1 + static_cast<bool>(isClosed));

				for (Vertex2D::IndexType k = 0; k < count; ++k)
				{
					for (Vertex2D::IndexType i = 0; i < 6; ++i)
					{
						*pIndex++ = (indexOffset + (detail::RectIndexTable[i] + k * 2) % vertexSize);
					}
				}
			}

			return indexSize;
		}

		Vertex2D::IndexType BuildPolygon(const BufferCreatorFunc& bufferCreator, const Array<Float2>& vertices, const Array<TriangleIndex>& tirnagleIndices, const Optional<Float2>& offset, const Float4& color)
		{
			if (vertices.isEmpty()
				|| tirnagleIndices.isEmpty())
			{
				return 0;
			}

			const Vertex2D::IndexType vertexSize = static_cast<Vertex2D::IndexType>(vertices.size());
			const Vertex2D::IndexType indexSize = static_cast<Vertex2D::IndexType>(tirnagleIndices.size() * 3);
			auto [pVertex, pIndex, indexOffset] = bufferCreator(vertexSize, indexSize);

			if (not pVertex)
			{
				return 0;
			}

			{
				const Float2* pSrc = vertices.data();
				const Float2* pSrcEnd = (pSrc + vertices.size());

				if (offset)
				{
					const Float2 _offset = offset.value();

					while (pSrc != pSrcEnd)
					{
						pVertex->pos = (_offset + *pSrc++);
						pVertex->color = color;
						++pVertex;
					}
				}
				else
				{
					while (pSrc != pSrcEnd)
					{
						pVertex->pos = *pSrc++;
						pVertex->color = color;
						++pVertex;
					}
				}
			}

			{
				std::memcpy(pIndex, tirnagleIndices.data(), tirnagleIndices.size_bytes());

				for (size_t i = 0; i < indexSize; ++i)
				{
					*(pIndex++) += indexOffset;
				}
			}

			return indexSize;
		}

		Vertex2D::IndexType BuildPolygon(const BufferCreatorFunc& bufferCreator, const Vertex2D* vertices, const size_t vertexCount, const TriangleIndex* indices, const size_t num_triangles)
		{
			if ((not vertices)
				|| (vertexCount == 0)
				|| (not indices)
				|| (num_triangles == 0))
			{
				return 0;
			}

			const Vertex2D::IndexType vertexSize = static_cast<Vertex2D::IndexType>(vertexCount);
			const Vertex2D::IndexType indexSize = static_cast<Vertex2D::IndexType>(num_triangles * 3);
			auto [pVertex, pIndex, indexOffset] = bufferCreator(vertexSize, indexSize);

			if (not pVertex)
			{
				return 0;
			}

			std::memcpy(pVertex, vertices, vertexSize * sizeof(Vertex2D));

			const TriangleIndex* pSrc = indices;
			const TriangleIndex* const pSrcEnd = (indices + num_triangles);

			while (pSrc != pSrcEnd)
			{
				*pIndex++ = (indexOffset + pSrc->i0);
				*pIndex++ = (indexOffset + pSrc->i1);
				*pIndex++ = (indexOffset + pSrc->i2);
				++pSrc;
			}

			return indexSize;
		}
	}
}