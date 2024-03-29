// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/public/cpp/app_list/app_list_config.h"
#include "ash/public/interfaces/app_list_view.mojom.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/strings/stringprintf.h"
#include "base/task/post_task.h"
#include "chrome/browser/ui/ash/ash_test_util.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/test/base/perf/drag_event_generator.h"
#include "chrome/test/base/perf/performance_test.h"
#include "ui/base/test/ui_controls.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"

// Test launcher drag performance in clamshell mode.
// TODO(oshima): Add test for tablet mode.
class LauncherDragTest : public UIPerformanceTest {
 public:
  LauncherDragTest() = default;
  ~LauncherDragTest() override = default;

  // UIPerformanceTest:
  void SetUpOnMainThread() override {
    UIPerformanceTest::SetUpOnMainThread();

    // Ash may not be ready to receive events right away.
    int warmup_seconds = base::SysInfo::IsRunningOnChromeOS() ? 5 : 1;
    base::RunLoop run_loop;
    base::PostDelayedTask(FROM_HERE, run_loop.QuitClosure(),
                          base::TimeDelta::FromSeconds(warmup_seconds));
    run_loop.Run();
  }

  // UIPerformanceTest:
  std::vector<std::string> GetUMAHistogramNames() const override {
    return {
        "Apps.StateTransition.Drag.PresentationTime.ClamshellMode",
    };
  }

  static gfx::Rect GetDisplayBounds(aura::Window* window) {
    return display::Screen::GetScreen()
        ->GetDisplayNearestWindow(window)
        .bounds();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(LauncherDragTest);
};

// Drag to open the launcher from shelf.
IN_PROC_BROWSER_TEST_F(LauncherDragTest, Open) {
  BrowserView* browser_view = BrowserView::GetBrowserViewForBrowser(browser());
  aura::Window* browser_window = browser_view->GetWidget()->GetNativeWindow();
  ash::mojom::ShellTestApiPtr shell_test_api = test::GetShellTestApi();

  base::RunLoop waiter;
  shell_test_api->WaitForLauncherAnimationState(
      ash::mojom::AppListViewState::kFullscreenAllApps, waiter.QuitClosure());

  gfx::Rect display_bounds = GetDisplayBounds(browser_window);
  // TODO(oshima): Use shelf constants.
  gfx::Point start_point =
      gfx::Point(display_bounds.width() / 4, display_bounds.bottom() - 28);
  gfx::Point end_point(start_point);
  end_point.set_y(10);
  ui_test_utils::DragEventGenerator generator(
      std::make_unique<ui_test_utils::InterporateProducer>(
          start_point, end_point, base::TimeDelta::FromMilliseconds(1000)),
      /*touch=*/true);
  generator.Wait();

  waiter.Run();
}

// Drag to close the launcher.
IN_PROC_BROWSER_TEST_F(LauncherDragTest, Close) {
  BrowserView* browser_view = BrowserView::GetBrowserViewForBrowser(browser());
  aura::Window* browser_window = browser_view->GetWidget()->GetNativeWindow();
  ash::mojom::ShellTestApiPtr shell_test_api = test::GetShellTestApi();
  {
    base::RunLoop waiter;
    shell_test_api->WaitForLauncherAnimationState(
        ash::mojom::AppListViewState::kFullscreenAllApps, waiter.QuitClosure());
    ui_controls::SendKeyPress(browser_window, ui::VKEY_BROWSER_SEARCH,
                              /*control=*/false,
                              /*shift=*/true,
                              /*alt=*/false,
                              /*command=*/false);
    waiter.Run();
  }

  base::RunLoop waiter;
  shell_test_api->WaitForLauncherAnimationState(
      ash::mojom::AppListViewState::kClosed, waiter.QuitClosure());

  gfx::Rect display_bounds = GetDisplayBounds(browser_window);
  gfx::Point start_point = gfx::Point(display_bounds.width() / 4, 10);
  gfx::Point end_point(start_point);
  // TODO(oshima): Use shelf_constants.
  end_point.set_y(display_bounds.bottom() - 56);
  ui_test_utils::DragEventGenerator generator(
      std::make_unique<ui_test_utils::InterporateProducer>(
          start_point, end_point, base::TimeDelta::FromMilliseconds(1000)),
      /*touch=*/true);
  generator.Wait();

  waiter.Run();
}
