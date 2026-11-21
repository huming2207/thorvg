/*
 * Copyright (c) 2021 - 2026 ThorVG project. All rights reserved.

 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <thorvg.h>
#include <fstream>
#include "config.h"
#include "testFramework.h"

using namespace tvg;
using namespace std;

#ifdef THORVG_CPU_ENGINE_SUPPORT

TEST_CASE("Basic draw", "[tvgSwEngine]")
{
    REQUIRE(Initializer::init() == Result::Success);
    {
        auto canvas = unique_ptr<SwCanvas>(SwCanvas::gen());
        REQUIRE(canvas);

        uint32_t buffer[100*100] = {};
        REQUIRE(canvas->target(buffer, 100, 100, 100, ColorSpace::ARGB8888S) == Result::Success);

        std::vector<MaskMethod> masks;
        masks.push_back(MaskMethod::None);
        masks.push_back(MaskMethod::Alpha);
        masks.push_back(MaskMethod::InvAlpha);
        masks.push_back(MaskMethod::Luma);
        masks.push_back(MaskMethod::InvLuma);
        masks.push_back(MaskMethod::Add);
        masks.push_back(MaskMethod::Subtract);
        masks.push_back(MaskMethod::Intersect);
        masks.push_back(MaskMethod::Difference);
        masks.push_back(MaskMethod::Lighten);
        masks.push_back(MaskMethod::Darken);

        std::vector<BlendMethod> methods;
        methods.push_back(BlendMethod::Normal);
        methods.push_back(BlendMethod::Multiply);
        methods.push_back(BlendMethod::Screen);
        methods.push_back(BlendMethod::Overlay);
        methods.push_back(BlendMethod::Darken);
        methods.push_back(BlendMethod::Lighten);
        methods.push_back(BlendMethod::ColorDodge);
        methods.push_back(BlendMethod::ColorBurn);
        methods.push_back(BlendMethod::HardLight);
        methods.push_back(BlendMethod::SoftLight);
        methods.push_back(BlendMethod::Difference);
        methods.push_back(BlendMethod::Hue);
        methods.push_back(BlendMethod::Saturation);
        methods.push_back(BlendMethod::Color);
        methods.push_back(BlendMethod::Luminosity);
        methods.push_back(BlendMethod::Add);
        methods.push_back(BlendMethod::Composition);

        auto mask = []() {
            auto mask = Shape::gen();
            mask->appendRect(0, 10, 20, 30, 5, 5);
            mask->opacity(127);
            mask->fill(255, 255, 255);
            return mask;
        };

        for (auto method : methods) {
            for (auto maskOp : masks) {
                //Arc Line
                auto shape1 = Shape::gen();
                REQUIRE(shape1->strokeFill(255, 255, 255, 255) == Result::Success);
                REQUIRE(shape1->strokeWidth(2) == Result::Success);
                REQUIRE(shape1->blend(method) == Result::Success);
                if (maskOp != MaskMethod::None) REQUIRE(shape1->mask(mask(), maskOp) == Result::Success);
                REQUIRE(canvas->add(shape1) == Result::Success);

                //Cubic
                auto shape2 = Shape::gen();
                REQUIRE(shape2->moveTo(50, 25) == Result::Success);
                REQUIRE(shape2->cubicTo(62, 25, 75, 38, 75, 50) == Result::Success);
                REQUIRE(shape2->close() == Result::Success);
                REQUIRE(shape2->strokeFill(255, 0, 0, 125) == Result::Success);
                REQUIRE(shape2->strokeWidth(1) == Result::Success);
                REQUIRE(shape2->blend(method) == Result::Success);
                if (maskOp != MaskMethod::None) REQUIRE(shape2->mask(mask(), maskOp) == Result::Success);
                REQUIRE(canvas->add(shape2) == Result::Success);

                //Fill
                auto shape3 = Shape::gen();
                REQUIRE(shape3->moveTo(0, 0) == Result::Success);
                REQUIRE(shape3->lineTo(20, 0) == Result::Success);
                REQUIRE(shape3->lineTo(20, 20) == Result::Success);
                REQUIRE(shape3->lineTo(0, 20) == Result::Success);
                REQUIRE(shape3->close() == Result::Success);
                REQUIRE(shape3->fill(255, 255, 255) == Result::Success);
                REQUIRE(shape3->blend(method) == Result::Success);
                if (maskOp != MaskMethod::None) REQUIRE(shape3->mask(mask(), maskOp) == Result::Success);
                REQUIRE(canvas->add(shape3) == Result::Success);

                //Dashed Line shape
                auto shape4 = Shape::gen();
                float dashPattern[2] = {2.5f, 5.0f};
                REQUIRE(shape4->moveTo(0, 0) == Result::Success);
                REQUIRE(shape4->lineTo(25, 25) == Result::Success);
                REQUIRE(shape4->cubicTo(50, 50, 75, -75, 50, 100) == Result::Success);
                REQUIRE(shape4->close() == Result::Success);
                REQUIRE(shape4->fill(255, 255, 255) == Result::Success);
                REQUIRE(shape4->strokeFill(255, 0, 0, 255) == Result::Success);
                REQUIRE(shape4->strokeWidth(2) == Result::Success);
                REQUIRE(shape4->strokeDash(dashPattern, 2) == Result::Success);
                REQUIRE(shape4->strokeCap(StrokeCap::Round) == Result::Success);
                REQUIRE(shape4->blend(method) == Result::Success);
                if (maskOp != MaskMethod::None) REQUIRE(shape4->mask(mask(), maskOp) == Result::Success);
                REQUIRE(canvas->add(shape4) == Result::Success);
            }
        }
        REQUIRE(canvas->draw(true) == Result::Success);
        REQUIRE(canvas->sync() == Result::Success);
    }
    REQUIRE(Initializer::term() == Result::Success);
}

TEST_CASE("Image Draw", "[tvgSwEngine]")
{
    REQUIRE(Initializer::init() == Result::Success);
    {
        auto canvas = unique_ptr<SwCanvas>(SwCanvas::gen());
        REQUIRE(canvas);

        uint32_t buffer[100*100] = {};
        REQUIRE(canvas->target(buffer, 100, 100, 100, ColorSpace::ARGB8888) == Result::Success);

        //raw image
        ifstream file(TEST_DIR"/rawimage_200x300.raw");
        if (!file.is_open()) return;
        auto data = (uint32_t*)malloc(sizeof(uint32_t) * (200*300));
        file.read(reinterpret_cast<char *>(data), sizeof (uint32_t) * 200 * 300);
        file.close();

        std::vector<MaskMethod> masks;
        masks.push_back(MaskMethod::None);
        masks.push_back(MaskMethod::Alpha);
        masks.push_back(MaskMethod::InvAlpha);
        masks.push_back(MaskMethod::Luma);
        masks.push_back(MaskMethod::InvLuma);
        masks.push_back(MaskMethod::Add);
        masks.push_back(MaskMethod::Subtract);
        masks.push_back(MaskMethod::Intersect);
        masks.push_back(MaskMethod::Difference);
        masks.push_back(MaskMethod::Lighten);
        masks.push_back(MaskMethod::Darken);

        std::vector<BlendMethod> methods;
        methods.push_back(BlendMethod::Normal);
        methods.push_back(BlendMethod::Multiply);
        methods.push_back(BlendMethod::Screen);
        methods.push_back(BlendMethod::Overlay);
        methods.push_back(BlendMethod::Darken);
        methods.push_back(BlendMethod::Lighten);
        methods.push_back(BlendMethod::ColorDodge);
        methods.push_back(BlendMethod::ColorBurn);
        methods.push_back(BlendMethod::HardLight);
        methods.push_back(BlendMethod::SoftLight);
        methods.push_back(BlendMethod::Difference);
        methods.push_back(BlendMethod::Hue);
        methods.push_back(BlendMethod::Saturation);
        methods.push_back(BlendMethod::Color);
        methods.push_back(BlendMethod::Luminosity);
        methods.push_back(BlendMethod::Add);
        methods.push_back(BlendMethod::Composition);

        auto mask = []() {
            auto mask = Shape::gen();
            mask->appendRect(0, 10, 20, 30, 5, 5);
            mask->fill(255, 255, 255);
            return mask;
        };

        for (auto method : methods) {
            for (auto maskOp : masks) {
                //Non-transformed images
                auto picture = Picture::gen();
                REQUIRE(picture->load(data, 200, 200, 300, ColorSpace::ARGB8888, true) == Result::Success);
                REQUIRE(picture->blend(method) == Result::Success);
                if (maskOp != MaskMethod::None) REQUIRE(picture->mask(mask(), maskOp) == Result::Success);
                REQUIRE(canvas->add(picture) == Result::Success);

                //Clipped images
                auto picture2 = picture->duplicate();
                REQUIRE(picture2->clip(mask()) == Result::Success);
                REQUIRE(canvas->add(picture2) == Result::Success);

                // Transformed images
                auto picture3 = picture->duplicate();
                REQUIRE(picture3->rotate(45) == Result::Success);
                REQUIRE(canvas->add(picture3) == Result::Success);

                //Up-scaled Image
                auto picture4 = picture->duplicate();
                REQUIRE(picture4->scale(2.0f) == Result::Success);
                REQUIRE(canvas->add(picture4) == Result::Success);

                //Down-scaled Image
                auto picture5 = picture->duplicate();
                REQUIRE(picture5->scale(0.25f) == Result::Success);
                REQUIRE(canvas->add(picture5) == Result::Success);

                //Direct Clipped image
                auto picture6 = Picture::gen();
                REQUIRE(picture6->load(data, 200, 200, 300, ColorSpace::ARGB8888, true) == Result::Success);
                REQUIRE(picture6->clip(mask()) == Result::Success);
                REQUIRE(picture6->blend(method) == Result::Success);
                REQUIRE(canvas->add(picture6) == Result::Success);

                //Scaled Clipped image
                auto picture7 = picture6->duplicate();
                REQUIRE(picture7->scale(2.0f) == Result::Success);
                REQUIRE(canvas->add(picture7) == Result::Success);
            }
        }

        REQUIRE(canvas->draw() == Result::Success);
        REQUIRE(canvas->sync() == Result::Success);
        free(data);
    }
    REQUIRE(Initializer::term() == Result::Success);
}

TEST_CASE("Filling Draw", "[tvgSwEngine]")
{
    REQUIRE(Initializer::init() == Result::Success);
    {
        auto canvas = unique_ptr<SwCanvas>(SwCanvas::gen());
        REQUIRE(canvas);

        uint32_t buffer[100*100] = {};
        REQUIRE(canvas->target(buffer, 100, 100, 100, ColorSpace::ARGB8888) == Result::Success);

        std::vector<MaskMethod> masks;
        masks.push_back(MaskMethod::None);
        masks.push_back(MaskMethod::Alpha);
        masks.push_back(MaskMethod::InvAlpha);
        masks.push_back(MaskMethod::Luma);
        masks.push_back(MaskMethod::InvLuma);
        masks.push_back(MaskMethod::Add);
        masks.push_back(MaskMethod::Subtract);
        masks.push_back(MaskMethod::Intersect);
        masks.push_back(MaskMethod::Difference);
        masks.push_back(MaskMethod::Lighten);
        masks.push_back(MaskMethod::Darken);

        std::vector<BlendMethod> methods;
        methods.push_back(BlendMethod::Normal);
        methods.push_back(BlendMethod::Multiply);
        methods.push_back(BlendMethod::Screen);
        methods.push_back(BlendMethod::Overlay);
        methods.push_back(BlendMethod::Darken);
        methods.push_back(BlendMethod::Lighten);
        methods.push_back(BlendMethod::ColorDodge);
        methods.push_back(BlendMethod::ColorBurn);
        methods.push_back(BlendMethod::HardLight);
        methods.push_back(BlendMethod::SoftLight);
        methods.push_back(BlendMethod::Difference);
        methods.push_back(BlendMethod::Hue);
        methods.push_back(BlendMethod::Saturation);
        methods.push_back(BlendMethod::Color);
        methods.push_back(BlendMethod::Luminosity);
        methods.push_back(BlendMethod::Add);
        methods.push_back(BlendMethod::Composition);

        auto mask = []() {
            auto mask = Shape::gen();
            mask->appendRect(10, 10, 20, 30, 5, 5);
            mask->opacity(127);
            mask->fill(255, 255, 255);
            return mask;
        };

        Fill::ColorStop cs[4] = {
            {0.1f, 0, 0, 0, 0},
            {0.2f, 50, 25, 50, 25},
            {0.5f, 100, 100, 100, 125},
            {0.9f, 255, 255, 255, 255}
        };

        for (auto method : methods) {
            for (auto maskOp : masks) {
                //Linear Gradient
                auto linear = LinearGradient::gen();
                REQUIRE(linear->colorStops(cs, 4) == Result::Success);
                REQUIRE(linear->spread(FillSpread::Repeat) == Result::Success);
                REQUIRE(linear->linear(0.0f, 0.0f, 100.0f, 120.0f) == Result::Success);

                auto shape = Shape::gen();
                REQUIRE(shape->appendRect(0, 0, 50, 50, 5, 5) == Result::Success);
                REQUIRE(shape->fill(linear) == Result::Success);
                REQUIRE(shape->blend(method) == Result::Success);
                if (maskOp != MaskMethod::None) REQUIRE(shape->mask(mask(), maskOp) == Result::Success);
                REQUIRE(canvas->add(shape) == Result::Success);

                //Radial Gradient
                auto radial = RadialGradient::gen();
                REQUIRE(radial->colorStops(cs, 4) == Result::Success);
                REQUIRE(radial->spread(FillSpread::Pad) == Result::Success);
                REQUIRE(radial->radial(50.0f, 50.0f, 50.0f, 50.0f, 50.0f, 0.0f) == Result::Success);

                auto shape2 = Shape::gen();
                REQUIRE(shape2->appendRect(50, 0, 50, 50) == Result::Success);
                REQUIRE(shape2->fill(radial) == Result::Success);
                REQUIRE(shape2->blend(method) == Result::Success);
                if (maskOp != MaskMethod::None) REQUIRE(shape2->mask(mask(), maskOp) == Result::Success);
                REQUIRE(canvas->add(shape2) == Result::Success);
            }
        }

        REQUIRE(canvas->draw() == Result::Success);
        REQUIRE(canvas->sync() == Result::Success);
    }
    REQUIRE(Initializer::term() == Result::Success);
}

TEST_CASE("Image Rotation", "[tvgSwEngine]")
{
    REQUIRE(Initializer::init() == Result::Success);
    {
        auto canvas = unique_ptr<SwCanvas>(SwCanvas::gen());
        REQUIRE(canvas);

        const uint32_t cw = 960;
        const uint32_t ch = 960;
        vector<uint32_t> buffer(static_cast<size_t>(cw) * ch);
        REQUIRE(canvas->target(buffer.data(), cw, ch, cw, ColorSpace::ARGB8888) == Result::Success);

        auto picture = Picture::gen();
        REQUIRE(picture);
 
        ifstream file(TEST_DIR"/rawimage_250x375.raw");
        if (!file.is_open()) return;
        auto data = (uint32_t*)malloc(sizeof(uint32_t) * (250*375));
        file.read(reinterpret_cast<char *>(data), sizeof (uint32_t) * 250 * 375);
        file.close();

        REQUIRE(picture->load(data, 250, 250, 375, ColorSpace::ARGB8888, true) == Result::Success);

        REQUIRE(picture->size(240, 240) == Result::Success);
        REQUIRE(picture->transform({0.572866f, -4.431353f, 336.605835f, 5.198910f, -0.386219f, 30.710693f, 0.0f, 0.0f, 1.0f}) == Result::Success);
        REQUIRE(canvas->add(picture) == Result::Success);

        REQUIRE(canvas->draw(true) == Result::Success);
        REQUIRE(canvas->sync() == Result::Success);

        free(data);
    }
    REQUIRE(Initializer::term() == Result::Success);
}

TEST_CASE("Intersection", "[tvgSwEngine]")
{
    REQUIRE(Initializer::init() == Result::Success);
    {
        auto canvas = unique_ptr<SwCanvas>(SwCanvas::gen());

        uint32_t buffer[200 * 200] = {};
        canvas->target(buffer, 200, 200, 200, ColorSpace::ARGB8888);

        auto shape = Shape::gen();
        REQUIRE(shape);
        REQUIRE(shape->appendRect(50, 50, 100, 100) == Result::Success);
        REQUIRE(shape->fill(255, 0, 0, 255) == Result::Success);

        REQUIRE(canvas->add(shape) == Result::Success);
        REQUIRE(canvas->draw() == Result::Success);

        // Case1. Fully contained
        REQUIRE(shape->intersects(0, 0, 200, 200, true) == true);

        // Case2. Partially overlapping
        REQUIRE(shape->intersects(25, 25, 50, 50, false) == true);
        REQUIRE(shape->intersects(125, 125, 50, 50, false) == true);

        shape->visible(false);

        // Case3. Edge-touching
        REQUIRE(shape->intersects(49, 49, 2, 2, true) == false);
        REQUIRE(shape->intersects(149, 149, 2, 2, false) == true);

        // Case4. Fully separated
        REQUIRE(shape->intersects(0, 0, 25, 25, true) == false);
        REQUIRE(shape->intersects(175, 175, 25, 25, true) == false);
    }
    REQUIRE(Initializer::term() == Result::Success);
}

TEST_CASE("Scene Effects Composition", "[tvgSwEngine]")
{
    REQUIRE(Initializer::init() == Result::Success);
    {
        constexpr uint32_t W = 64, H = 64;
        uint32_t direct[W * H], indirect[W * H];

        // A second, inactive effect (a zero sigma blur) composes the scene effect via an intermediate layer.
        auto draw = [&](uint32_t* buffer, int effect, bool layered, bool dirty) {
            auto canvas = unique_ptr<SwCanvas>(SwCanvas::gen());
            REQUIRE(canvas->target(buffer, W, W, H, ColorSpace::ARGB8888) == Result::Success);

            // leaves the shadow of a full region in the pooled post processing buffers
            if (dirty) {
                auto scene = Scene::gen();
                auto shape = Shape::gen();
                shape->appendRect(0, 0, W, H);
                shape->fill(255, 0, 0);
                scene->add(shape);
                scene->add(SceneEffect::DropShadow, 0, 255, 0, 255, 45.0, 3.0, 3.0, 100);
                scene->add(SceneEffect::GaussianBlur, 0.0, 0, 0, 100);
                REQUIRE(canvas->add(scene) == Result::Success);
                REQUIRE(canvas->draw(true) == Result::Success);
                REQUIRE(canvas->sync() == Result::Success);
                REQUIRE(canvas->remove() == Result::Success);
            }

            auto scene = Scene::gen();
            auto shape = Shape::gen();
            shape->appendRect(16, 16, 24, 24);
            shape->fill(200, 60, 30);
            scene->add(shape);
            scene->opacity(effect == 0 ? 255 : 128);
            if (effect == 0) scene->add(SceneEffect::DropShadow, 0, 0, 0, 128, 45.0, 6.0, 0.0, 100);
            if (effect == 1) scene->add(SceneEffect::Tint, 0, 0, 0, 255, 255, 255, 60.0);
            if (effect == 2) scene->add(SceneEffect::Tritone, 0, 0, 0, 128, 128, 128, 255, 255, 255, 0);
            if (layered) scene->add(SceneEffect::GaussianBlur, 0.0, 0, 0, 100);
            REQUIRE(canvas->add(scene) == Result::Success);
            REQUIRE(canvas->draw(true) == Result::Success);
            REQUIRE(canvas->sync() == Result::Success);
        };

        auto maxDiff = [&]() {
            int diff = 0;
            for (uint32_t i = 0; i < W * H; ++i) {
                for (int s = 0; s < 32; s += 8)
                    diff = std::max(diff, abs(int((direct[i] >> s) & 0xff) - int((indirect[i] >> s) & 0xff)));
            }
            return diff;
        };

        // DropShadow without blur keeps the shadow opacity and clears the uncovered region of the layer
        draw(direct, 0, false, false);
        draw(indirect, 0, true, true);
        REQUIRE(maxDiff() <= 1);

        // Tint, Tritone: the scene opacity is applied once
        for (int effect = 1; effect <= 2; ++effect) {
            draw(direct, effect, false, false);
            draw(indirect, effect, true, false);
            REQUIRE(maxDiff() <= 1);
        }
    }
    REQUIRE(Initializer::term() == Result::Success);
}

TEST_CASE("Grayscale8 Draw", "[tvgSwEngine]")
{
    REQUIRE(Initializer::init() == Result::Success);
    {
        auto canvas = unique_ptr<SwCanvas>(SwCanvas::gen());
        REQUIRE(canvas);

        uint8_t buffer[100 * 100] = {};
        REQUIRE(canvas->target((uint32_t*)buffer, 100, 100, 100, ColorSpace::Grayscale8) == Result::Success);

        auto pixel = [&](int x, int y) {
            return int(buffer[y * 100 + x]);
        };

        // Shapes
        auto bg = Shape::gen();
        REQUIRE(bg->appendRect(0, 0, 100, 100) == Result::Success);
        REQUIRE(bg->fill(255, 255, 255) == Result::Success);
        REQUIRE(canvas->add(bg) == Result::Success);

        auto solid = Shape::gen();
        REQUIRE(solid->appendRect(0, 0, 50, 50) == Result::Success);
        REQUIRE(solid->fill(0, 0, 0) == Result::Success);
        REQUIRE(canvas->add(solid) == Result::Success);

        auto translucent = Shape::gen();
        REQUIRE(translucent->appendRect(50, 0, 50, 50) == Result::Success);
        REQUIRE(translucent->fill(0, 0, 0, 128) == Result::Success);
        REQUIRE(canvas->add(translucent) == Result::Success);

        auto circle = Shape::gen();
        REQUIRE(circle->appendCircle(25, 75, 20, 20) == Result::Success);
        REQUIRE(circle->fill(255, 0, 0) == Result::Success);
        REQUIRE(canvas->add(circle) == Result::Success);

        auto scene = Scene::gen();
        auto rect = Shape::gen();
        REQUIRE(rect->appendRect(50, 50, 50, 50) == Result::Success);
        REQUIRE(rect->fill(0, 0, 0) == Result::Success);
        REQUIRE(scene->add(rect) == Result::Success);
        REQUIRE(scene->opacity(128) == Result::Success);
        REQUIRE(canvas->add(scene) == Result::Success);

        REQUIRE(canvas->draw() == Result::Success);
        REQUIRE(canvas->sync() == Result::Success);

        REQUIRE(pixel(25, 25) == 0);
        REQUIRE(pixel(75, 25) == 127);
        REQUIRE(pixel(25, 75) == 54);  // red luminance (54.2)
        REQUIRE(abs(pixel(75, 75) - 127) <= 1);
        REQUIRE(pixel(2, 98) == 255);

        // Masking, Gradient
        REQUIRE(canvas->remove() == Result::Success);

        auto shape = Shape::gen();
        REQUIRE(shape->appendRect(0, 0, 100, 50) == Result::Success);
        REQUIRE(shape->fill(255, 255, 255) == Result::Success);
        auto mask = Shape::gen();
        REQUIRE(mask->appendRect(0, 0, 50, 50) == Result::Success);
        REQUIRE(mask->fill(255, 255, 255) == Result::Success);
        REQUIRE(shape->mask(mask, MaskMethod::Alpha) == Result::Success);
        REQUIRE(canvas->add(shape) == Result::Success);

        Fill::ColorStop cs[2] = {{0.0f, 0, 0, 0, 255}, {1.0f, 255, 255, 255, 255}};
        auto linear = LinearGradient::gen();
        REQUIRE(linear->colorStops(cs, 2) == Result::Success);
        REQUIRE(linear->linear(0.0f, 0.0f, 100.0f, 0.0f) == Result::Success);
        auto shape2 = Shape::gen();
        REQUIRE(shape2->appendRect(0, 50, 100, 50) == Result::Success);
        REQUIRE(shape2->fill(linear) == Result::Success);
        REQUIRE(canvas->add(shape2) == Result::Success);

        REQUIRE(canvas->draw(true) == Result::Success);
        REQUIRE(canvas->sync() == Result::Success);

        REQUIRE(pixel(25, 25) == 255);
        REQUIRE(pixel(75, 25) == 0);
        REQUIRE(pixel(5, 75) < pixel(50, 75));
        REQUIRE(pixel(50, 75) < pixel(95, 75));
        REQUIRE(abs(pixel(50, 75) - 128) <= 2);

        // Images: direct, scaled, transformed
        REQUIRE(canvas->remove() == Result::Success);

        uint32_t data[10 * 10];
        for (int i = 0; i < 10 * 10; ++i)
            data[i] = 0xff00ff00;  // opaque green

        auto picture = Picture::gen();
        REQUIRE(picture->load(data, 10, 10, 10, ColorSpace::ARGB8888, true) == Result::Success);
        REQUIRE(canvas->add(picture) == Result::Success);

        auto picture2 = picture->duplicate();
        REQUIRE(picture2->translate(20, 0) == Result::Success);
        REQUIRE(picture2->scale(2.0f) == Result::Success);
        REQUIRE(canvas->add(picture2) == Result::Success);

        auto picture3 = picture->duplicate();
        REQUIRE(picture3->translate(60, 30) == Result::Success);
        REQUIRE(picture3->rotate(30.0f) == Result::Success);
        REQUIRE(canvas->add(picture3) == Result::Success);

        REQUIRE(canvas->draw(true) == Result::Success);
        REQUIRE(canvas->sync() == Result::Success);

        REQUIRE(pixel(5, 5) == 182);  // green luminance
        REQUIRE(pixel(30, 10) == 182);
        REQUIRE(pixel(61, 36) == 182);
        REQUIRE(pixel(50, 90) == 0);

        // Opaque image with opacity over a gray background
        REQUIRE(canvas->remove() == Result::Success);

        auto bg2 = Shape::gen();
        REQUIRE(bg2->appendRect(0, 0, 100, 100) == Result::Success);
        REQUIRE(bg2->fill(100, 100, 100) == Result::Success);
        REQUIRE(canvas->add(bg2) == Result::Success);

        // the user owned data has been converted in place by the previous pictures
        for (int i = 0; i < 10 * 10; ++i) {
            data[i] = 0xff00ff00;
        }
        auto picture4 = Picture::gen();
        REQUIRE(picture4->load(data, 10, 10, 10, ColorSpace::XRGB8888, true) == Result::Success);
        REQUIRE(picture4->opacity(128) == Result::Success);
        REQUIRE(canvas->add(picture4) == Result::Success);

        auto blend = Shape::gen();
        REQUIRE(blend->appendRect(60, 60, 40, 40) == Result::Success);
        REQUIRE(blend->fill(0, 0, 255) == Result::Success);
        REQUIRE(blend->blend(BlendMethod::Multiply) == Result::Success);
        REQUIRE(canvas->add(blend) == Result::Success);

        // the same gray with opacity keeps the gray (rounded once)
        auto bg3 = Shape::gen();
        REQUIRE(bg3->appendRect(20, 20, 10, 10) == Result::Success);
        REQUIRE(bg3->fill(101, 101, 101) == Result::Success);
        REQUIRE(canvas->add(bg3) == Result::Success);

        uint32_t gray[10 * 10];
        for (int i = 0; i < 10 * 10; ++i) {
            gray[i] = 0xff656565;
        }
        auto picture5 = Picture::gen();
        REQUIRE(picture5->load(gray, 10, 10, 10, ColorSpace::XRGB8888, true) == Result::Success);
        REQUIRE(picture5->translate(20, 20) == Result::Success);
        REQUIRE(picture5->opacity(128) == Result::Success);
        REQUIRE(canvas->add(picture5) == Result::Success);

        REQUIRE(canvas->draw(true) == Result::Success);
        REQUIRE(canvas->sync() == Result::Success);

        REQUIRE(abs(pixel(5, 5) - 141) <= 1);
        REQUIRE(pixel(50, 50) == 100);
        REQUIRE(pixel(80, 80) == 7);  // blue x gray(100) luminance
        REQUIRE(pixel(25, 25) == 101);

        // Stacked translucent layers darken as on the 32 bits target, without stalling
        uint32_t argb[100 * 100];
        auto canvas2 = unique_ptr<SwCanvas>(SwCanvas::gen());
        REQUIRE(canvas2->target(argb, 100, 100, 100, ColorSpace::ARGB8888) == Result::Success);

        for (auto c : {canvas.get(), canvas2.get()}) {
            REQUIRE(c->remove() == Result::Success);
            auto white = Shape::gen();
            REQUIRE(white->appendRect(0, 0, 100, 100) == Result::Success);
            REQUIRE(white->fill(255, 255, 255) == Result::Success);
            REQUIRE(c->add(white) == Result::Success);
            for (int i = 0; i < 200; ++i) {
                auto layer = Shape::gen();
                REQUIRE(layer->appendRect(0, 0, 50, 100) == Result::Success);
                REQUIRE(layer->fill(0, 0, 0, i < 100 ? 5 : 2) == Result::Success);
                REQUIRE(c->add(layer) == Result::Success);
            }
            REQUIRE(c->draw(true) == Result::Success);
            REQUIRE(c->sync() == Result::Success);
        }
        REQUIRE(pixel(25, 50) == int(argb[50 * 100 + 25] & 0xff));
        REQUIRE(pixel(25, 50) < 5);
        REQUIRE(pixel(75, 50) == 255);
    }
    REQUIRE(Initializer::term() == Result::Success);
}

TEST_CASE("Grayscale8 Composition", "[tvgSwEngine]")
{
    REQUIRE(Initializer::init() == Result::Success);
    {
        auto canvas = unique_ptr<SwCanvas>(SwCanvas::gen());
        REQUIRE(canvas);

        uint8_t buffer[100 * 100] = {};
        REQUIRE(canvas->target((uint32_t*)buffer, 100, 100, 100, ColorSpace::Grayscale8) == Result::Success);

        // raw image
        ifstream file(TEST_DIR "/rawimage_200x300.raw");
        if (!file.is_open()) return;
        auto data = (uint32_t*)malloc(sizeof(uint32_t) * (200 * 300));
        file.read(reinterpret_cast<char*>(data), sizeof(uint32_t) * 200 * 300);
        file.close();

        MaskMethod masks[] = {MaskMethod::None, MaskMethod::Alpha, MaskMethod::InvAlpha, MaskMethod::Luma, MaskMethod::InvLuma, MaskMethod::Add, MaskMethod::Subtract, MaskMethod::Intersect, MaskMethod::Difference, MaskMethod::Lighten, MaskMethod::Darken};
        BlendMethod methods[] = {BlendMethod::Normal, BlendMethod::Multiply, BlendMethod::Screen, BlendMethod::Overlay, BlendMethod::Darken, BlendMethod::Lighten, BlendMethod::ColorDodge, BlendMethod::ColorBurn, BlendMethod::HardLight, BlendMethod::SoftLight, BlendMethod::Difference, BlendMethod::Hue, BlendMethod::Saturation, BlendMethod::Color, BlendMethod::Luminosity, BlendMethod::Add, BlendMethod::Composition};

        auto mask = []() {
            auto mask = Shape::gen();
            mask->appendRect(10, 10, 20, 30, 5, 5);
            mask->opacity(127);
            mask->fill(255, 255, 255);
            return mask;
        };

        Fill::ColorStop cs[3] = {{0.1f, 0, 0, 0, 0}, {0.5f, 100, 200, 50, 125}, {0.9f, 255, 255, 255, 255}};

        for (auto method : methods) {
            for (auto maskOp : masks) {
                auto shape = Shape::gen();
                REQUIRE(shape->appendCircle(30, 30, 25, 25) == Result::Success);
                REQUIRE(shape->fill(200, 100, 50, 200) == Result::Success);
                REQUIRE(shape->strokeFill(255, 0, 0, 125) == Result::Success);
                REQUIRE(shape->strokeWidth(3) == Result::Success);
                REQUIRE(shape->blend(method) == Result::Success);
                if (maskOp != MaskMethod::None) REQUIRE(shape->mask(mask(), maskOp) == Result::Success);
                REQUIRE(canvas->add(shape) == Result::Success);

                auto rect = Shape::gen();
                REQUIRE(rect->appendRect(40, 40, 50, 50) == Result::Success);
                REQUIRE(rect->fill(0, 128, 255) == Result::Success);
                REQUIRE(rect->blend(method) == Result::Success);
                if (maskOp != MaskMethod::None) REQUIRE(rect->mask(mask(), maskOp) == Result::Success);
                REQUIRE(canvas->add(rect) == Result::Success);

                auto radial = RadialGradient::gen();
                REQUIRE(radial->colorStops(cs, 3) == Result::Success);
                REQUIRE(radial->radial(50.0f, 50.0f, 50.0f, 50.0f, 50.0f, 0.0f) == Result::Success);
                auto shape2 = Shape::gen();
                REQUIRE(shape2->appendRect(0, 50, 100, 50, 10, 10) == Result::Success);
                REQUIRE(shape2->fill(radial) == Result::Success);
                REQUIRE(shape2->blend(method) == Result::Success);
                if (maskOp != MaskMethod::None) REQUIRE(shape2->mask(mask(), maskOp) == Result::Success);
                REQUIRE(canvas->add(shape2) == Result::Success);

                auto picture = Picture::gen();
                REQUIRE(picture->load(data, 200, 200, 300, ColorSpace::ARGB8888, true) == Result::Success);
                REQUIRE(picture->blend(method) == Result::Success);
                if (maskOp != MaskMethod::None) REQUIRE(picture->mask(mask(), maskOp) == Result::Success);
                REQUIRE(canvas->add(picture) == Result::Success);

                auto picture2 = picture->duplicate();
                REQUIRE(picture2->clip(mask()) == Result::Success);
                REQUIRE(canvas->add(picture2) == Result::Success);

                auto picture3 = picture->duplicate();
                REQUIRE(picture3->rotate(45) == Result::Success);
                REQUIRE(canvas->add(picture3) == Result::Success);

                auto picture4 = picture->duplicate();
                REQUIRE(picture4->scale(0.25f) == Result::Success);
                REQUIRE(canvas->add(picture4) == Result::Success);

                auto picture5 = picture->duplicate();
                REQUIRE(picture5->scale(2.0f) == Result::Success);
                REQUIRE(picture5->clip(mask()) == Result::Success);
                REQUIRE(canvas->add(picture5) == Result::Success);

                auto scene = Scene::gen();
                REQUIRE(scene->add(picture->duplicate()) == Result::Success);
                REQUIRE(scene->add(shape->duplicate()) == Result::Success);
                REQUIRE(scene->opacity(100) == Result::Success);
                REQUIRE(scene->blend(method) == Result::Success);
                REQUIRE(scene->add(SceneEffect::GaussianBlur, 2.0, 0, 0, 50) == Result::Success);
                REQUIRE(canvas->add(scene) == Result::Success);
            }
        }

        REQUIRE(canvas->draw() == Result::Success);
        REQUIRE(canvas->sync() == Result::Success);
        free(data);
    }
    REQUIRE(Initializer::term() == Result::Success);
}

#endif