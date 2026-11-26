#if defined(DEATH_ENABLE_NEON)
		// NOTE ON EXACTNESS: unlike the other variants in this file, this one is NOT bit-identical to the
		// Cpu::ScalarT sibling, and that is deliberate.
		//
		// The scalar form divides by 255 and multiplies by 255 again at the end. ARMv7 NEON has no vector
		// divide (that arrived with ARMv8), so the choice was between four scalar VFP divides per vector -
		// which gives most of the vectorisation back - and folding the two constants out algebraically:
		//
		//     c   = (px/255)*lit + core
		//     c2  = c + (amb - c)*darkT   = c*(1 - darkT) + amb*darkT
		//     out = c2*255 + 0.5          = px*[lit*(1-darkT)] + [core*(1-darkT) + amb*darkT]*255 + 0.5
		//
		// so with A = lit*(1-darkT) and B = (core*(1-darkT) + amb*darkT)*255 + 0.5 the whole thing becomes a
		// clamped multiply-add on the raw byte and the division never happens. Same function of the same
		// inputs, but a different order of floating-point operations, so a result can land one ulp from the
		// scalar one and quantize to a neighbouring byte. In a lighting term that is invisible.
		//
		// Written for the Miyoo port because this op is ~36% of the frame's CPU time there (76800 px every
		// frame, measured) and was falling back to scalar for want of a NEON variant.
		//
		// Inactive pixels need no masking: with darkT = 0 and g = 0 the formula collapses to A = 1, B = 0.5,
		// i.e. out = trunc(px + 0.5) = px. The early-out below is therefore only a speed optimization.
		DEATH_CPU_MAYBE_UNUSED DEATH_ENABLE_NEON typename std::decay<decltype(combineLightingScanline)>::type combineLightingScanlineImplementation(Cpu::NeonT) {
			return [](std::uint8_t* DEATH_RESTRICT px, std::int32_t width, const float* DEATH_RESTRICT lmRow, std::int32_t lmW, std::int32_t scale, float ambR, float ambG, float ambB) DEATH_ENABLE_NEON {
				const float32x4_t zerof = vdupq_n_f32(0.0f);
				const float32x4_t onef = vdupq_n_f32(1.0f);
				const float32x4_t halff = vdupq_n_f32(0.5f);
				const float32x4_t c255f = vdupq_n_f32(255.0f);
				const float32x4_t coreT = vdupq_n_f32(0.7f);
				const float32x4_t vAmbR255 = vdupq_n_f32(ambR * 255.0f);
				const float32x4_t vAmbG255 = vdupq_n_f32(ambG * 255.0f);
				const float32x4_t vAmbB255 = vdupq_n_f32(ambB * 255.0f);

				// x / scale collapses to a shift when the scale is a power of two (this port uses 4)
				std::int32_t shift = -1;
				if ((scale & (scale - 1)) == 0) {
					shift = 0;
					while ((1 << shift) < scale) shift++;
				}
				const std::int32_t lmLast = lmW - 1;

				std::int32_t x = 0;
				for (; x + 8 <= width; x += 8, px += 32) {
					// Gather the eight lightmap texels this run samples. Scalar on purpose: the indices repeat
					// in runs of `scale` but are not necessarily aligned to the vector, and a shuffle-based
					// broadcast would cost more than eight loads.
					float rArr[8], gArr[8];
					for (std::int32_t k = 0; k < 8; k++) {
						std::int32_t i = (shift >= 0 ? (x + k) >> shift : (x + k) / scale);
						if (i > lmLast) {
							i = lmLast;
						}
						rArr[k] = lmRow[i * 2];
						gArr[k] = lmRow[i * 2 + 1];
					}

					const uint8x8x4_t pix = vld4_u8(px);
					const uint16x8_t r16 = vmovl_u8(pix.val[0]);
					const uint16x8_t g16 = vmovl_u8(pix.val[1]);
					const uint16x8_t b16 = vmovl_u8(pix.val[2]);
					uint16x4_t qr[2], qg[2], qb[2];
					bool touched[2] = { false, false };

					for (std::int32_t half = 0; half < 2; half++) {
						float32x4_t r = vld1q_f32(&rArr[half * 4]);
						float32x4_t g = vld1q_f32(&gArr[half * 4]);
						r = vminq_f32(vmaxq_f32(r, zerof), onef);
						g = vminq_f32(vmaxq_f32(g, zerof), onef);
						const float32x4_t darkT = vminq_f32(vmaxq_f32(vsubq_f32(onef, r), zerof), onef);

						// Per-4-pixel early out (the block form of the scalar per-pixel fully-lit skip)
						const uint32x4_t active = vorrq_u32(vcgtq_f32(darkT, zerof), vcgtq_f32(g, zerof));
						const uint32x2_t act2 = vorr_u32(vget_low_u32(active), vget_high_u32(active));
						if ((vget_lane_u32(act2, 0) | vget_lane_u32(act2, 1)) == 0) {
							continue;
						}
						touched[half] = true;

						const float32x4_t lit = vaddq_f32(onef, g);
						const float32x4_t core = vmaxq_f32(vsubq_f32(g, coreT), zerof);
						const float32x4_t keep = vsubq_f32(onef, darkT);
						const float32x4_t vA = vmulq_f32(lit, keep);
						const float32x4_t bCommon = vaddq_f32(vmulq_f32(vmulq_f32(core, keep), c255f), halff);
						const float32x4_t bR = vaddq_f32(bCommon, vmulq_f32(vAmbR255, darkT));
						const float32x4_t bG = vaddq_f32(bCommon, vmulq_f32(vAmbG255, darkT));
						const float32x4_t bB = vaddq_f32(bCommon, vmulq_f32(vAmbB255, darkT));

						const uint32x4_t ru = (half == 0 ? vmovl_u16(vget_low_u16(r16)) : vmovl_u16(vget_high_u16(r16)));
						const uint32x4_t gu = (half == 0 ? vmovl_u16(vget_low_u16(g16)) : vmovl_u16(vget_high_u16(g16)));
						const uint32x4_t bu = (half == 0 ? vmovl_u16(vget_low_u16(b16)) : vmovl_u16(vget_high_u16(b16)));

						float32x4_t fr = vaddq_f32(vmulq_f32(vcvtq_f32_u32(ru), vA), bR);
						float32x4_t fg = vaddq_f32(vmulq_f32(vcvtq_f32_u32(gu), vA), bG);
						float32x4_t fb = vaddq_f32(vmulq_f32(vcvtq_f32_u32(bu), vA), bB);
						fr = vminq_f32(vmaxq_f32(fr, zerof), c255f);
						fg = vminq_f32(vmaxq_f32(fg, zerof), c255f);
						fb = vminq_f32(vmaxq_f32(fb, zerof), c255f);

						// vcvtq_u32_f32 truncates toward zero, which is what the scalar cast does after its clamp
						qr[half] = vmovn_u32(vcvtq_u32_f32(fr));
						qg[half] = vmovn_u32(vcvtq_u32_f32(fg));
						qb[half] = vmovn_u32(vcvtq_u32_f32(fb));
					}

					if (!touched[0] && !touched[1]) {
						continue;
					}
					// Halves that were skipped keep their loaded bytes
					if (!touched[0]) {
						qr[0] = vget_low_u16(r16); qg[0] = vget_low_u16(g16); qb[0] = vget_low_u16(b16);
					}
					if (!touched[1]) {
						qr[1] = vget_high_u16(r16); qg[1] = vget_high_u16(g16); qb[1] = vget_high_u16(b16);
					}

					uint8x8x4_t outPix;
					outPix.val[0] = vmovn_u16(vcombine_u16(qr[0], qr[1]));
					outPix.val[1] = vmovn_u16(vcombine_u16(qg[0], qg[1]));
					outPix.val[2] = vmovn_u16(vcombine_u16(qb[0], qb[1]));
					outPix.val[3] = pix.val[3];	// alpha untouched, exactly as the other variants leave it
					vst4_u8(px, outPix);
				}
				// Scalar tail - the reference form, so the last few pixels stay bit-identical
				for (; x < width; x++, px += 4) {
					const std::int32_t lmX = std::min(x / scale, lmW - 1);
					const float r = std::clamp(lmRow[lmX * 2], 0.0f, 1.0f);
					const float g = std::clamp(lmRow[lmX * 2 + 1], 0.0f, 1.0f);
					const float darkT = std::clamp(1.0f - r, 0.0f, 1.0f);
					if (darkT <= 0.0f && g <= 0.0f) {
						continue;
					}
					const float lit = 1.0f + g;
					const float core = (g > 0.7f ? g - 0.7f : 0.0f);
					float cr = (px[0] / 255.0f) * lit + core;
					float cg = (px[1] / 255.0f) * lit + core;
					float cb = (px[2] / 255.0f) * lit + core;
					cr += (ambR - cr) * darkT;
					cg += (ambG - cg) * darkT;
					cb += (ambB - cb) * darkT;
					px[0] = (std::uint8_t)std::clamp(cr * 255.0f + 0.5f, 0.0f, 255.0f);
					px[1] = (std::uint8_t)std::clamp(cg * 255.0f + 0.5f, 0.0f, 255.0f);
					px[2] = (std::uint8_t)std::clamp(cb * 255.0f + 0.5f, 0.0f, 255.0f);
				}
			};
		}
#endif

