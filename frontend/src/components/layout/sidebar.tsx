import { type Component, createSignal, For, type JSX, onCleanup, onMount, Show } from "solid-js";

import { useStore } from "../../contexts/store";
import { ToggleScheduleButton } from "../../scheduler";

interface SidebarSectionProps {
  title: string;
  children: JSX.Element;
}

const SidebarSection: Component<SidebarSectionProps> = (props) => (
  <div class="space-y-3">
    <h3 class="text-sm font-semibold text-slate-700 dark:text-slate-300 uppercase tracking-wide">
      {props.title}
    </h3>
    <div class="space-y-2">{props.children}</div>
  </div>
);

interface SidebarProps {
  onRotate: (turnRight: boolean) => void;
  onLoadImage: () => void;
  onClear: () => void;
  onPersist: () => void;
  onLoad: () => void;
  onPluginChange: (pluginId: number) => void;
  onBrightnessChange: (value: number, shouldSend?: boolean) => void;
  onArtnetChange: (value: number, shouldSend?: boolean) => void;
  onPersistPlugin: () => void;
  onSpeedChange: (value: number, shouldSend?: boolean) => void;
}

export const Sidebar: Component<SidebarProps> = (props) => {
  const [store] = useStore();

  const [currentHash, setCurrentHash] = createSignal(window.location.hash);
  const [isDark, setIsDark] = createSignal(false);

  onMount(() => {
    const handler = () => setCurrentHash(window.location.hash);
    window.addEventListener("hashchange", handler);

    const storedTheme = localStorage.getItem("theme");
    if (
      storedTheme === "dark" ||
      (!storedTheme && window.matchMedia("(prefers-color-scheme: dark)").matches)
    ) {
      setIsDark(true);
      document.documentElement.classList.add("dark");
    } else {
      setIsDark(false);
      document.documentElement.classList.remove("dark");
    }

    onCleanup(() => window.removeEventListener("hashchange", handler));
  });

  const getHref = (path: string) => {
    return currentHash() === `#${path}` ? "#/" : `#${path}`;
  };

  const toggleTheme = () => {
    const newDark = !isDark();
    setIsDark(newDark);
    if (newDark) {
      document.documentElement.classList.add("dark");
      localStorage.setItem("theme", "dark");
    } else {
      document.documentElement.classList.remove("dark");
      localStorage.setItem("theme", "light");
    }
  };

  return (
    <>
      <div class="flex-1 min-h-0 overflow-y-auto">
        <Show
          when={!store?.isActiveScheduler}
          fallback={
            <Show when={store.schedule.length > 0}>
              <ToggleScheduleButton />
            </Show>
          }
        >
          <SidebarSection title="Display Mode">
            <div class="flex flex-col gap-2.5">
              <select
                class="flex-1 px-2.5 py-2.5 bg-slate-50 dark:bg-slate-700 dark:text-white border border-slate-200 dark:border-slate-600 rounded outline-none focus:ring-2 focus:ring-slate-500"
                onChange={(e) => props.onPluginChange(parseInt(e.currentTarget.value, 10))}
                value={store?.plugin}
              >
                <For each={store?.plugins}>
                  {(plugin) => <option value={plugin.id}>{plugin.name}</option>}
                </For>
              </select>
              <button
                type="button"
                onClick={props.onPersistPlugin}
                class="w-full bg-slate-800 dark:bg-slate-700 text-white border-0 px-3 py-2 text-sm cursor-pointer font-semibold hover:opacity-80 active:-translate-y-px transition-all rounded shadow-sm"
              >
                Set as Default
              </button>
            </div>
          </SidebarSection>
        </Show>

        <div class="my-6 border-t border-slate-200 dark:border-slate-700" />

        <SidebarSection title={`Rotation (${[0, 90, 180, 270][store?.rotation || 0]}°)`}>
          <div class="flex gap-2.5">
            <button
              type="button"
              onClick={() => props.onRotate(false)}
              class="w-full bg-slate-800 dark:bg-slate-700 text-white border-0 px-3 py-2 text-sm cursor-pointer font-semibold hover:opacity-80 active:-translate-y-px transition-all rounded flex items-center justify-center gap-2 shadow-sm"
            >
              <i class="fa-solid fa-rotate-left" />
              <span class="hidden xl:inline">Left</span>
            </button>
            <button
              type="button"
              onClick={() => props.onRotate(true)}
              class="w-full bg-slate-800 dark:bg-slate-700 text-white border-0 px-3 py-2 text-sm cursor-pointer font-semibold hover:opacity-80 active:-translate-y-px transition-all rounded flex items-center justify-center gap-2 shadow-sm"
            >
              <i class="fa-solid fa-rotate-right" />
              <span class="hidden xl:inline">Right</span>
            </button>
          </div>
        </SidebarSection>

        <div class="my-6 border-t border-slate-200 dark:border-slate-700" />

        <SidebarSection title="Brightness">
          <div class="space-y-2">
            <input
              type="range"
              min="0"
              max="255"
              value={store?.brightness}
              class="w-full"
              onInput={(e) => props.onBrightnessChange(parseInt(e.currentTarget.value, 10))}
              onPointerUp={(e) =>
                props.onBrightnessChange(parseInt(e.currentTarget.value, 10), true)
              }
            />
            <div class="text-sm text-slate-500 dark:text-slate-400 text-right font-medium">
              {Math.round(((store?.brightness ?? 255) / 255) * 100)}%
            </div>
          </div>
        </SidebarSection>

        <Show when={store?.plugin === 17 && !store?.isActiveScheduler}>
          <div class="my-6 border-t border-slate-200 dark:border-slate-700" />

          <SidebarSection title="ArtNet Universe">
            <div class="space-y-2">
              <input
                type="range"
                min="0"
                max="255"
                value={store?.artnetUniverse}
                class="w-full"
                onInput={(e) => props.onArtnetChange(parseInt(e.currentTarget.value, 10))}
                onPointerUp={(e) => props.onArtnetChange(parseInt(e.currentTarget.value, 10), true)}
              />
              <div class="text-sm text-slate-500 dark:text-slate-400 text-right font-medium">
                {store?.artnetUniverse}
              </div>
            </div>
          </SidebarSection>
        </Show>

        <Show when={store?.hasSpeedControl && !store?.isActiveScheduler}>
          <div class="my-6 border-t border-slate-200 dark:border-slate-700" />

          <SidebarSection title="Animation Speed">
            <div class="space-y-2">
              <div class="flex items-center gap-2">
                <input
                  type="range"
                  min="1"
                  max="100"
                  value={store?.speed}
                  class="flex-1"
                  onInput={(e) => props.onSpeedChange(parseInt(e.currentTarget.value, 10))}
                  onPointerUp={(e) =>
                    props.onSpeedChange(parseInt(e.currentTarget.value, 10), true)
                  }
                />
                <button
                  type="button"
                  onClick={() => props.onSpeedChange(store?.defaultSpeed || 50, true)}
                  class="bg-slate-800 dark:bg-slate-700 text-white border-0 px-2 py-1 text-xs cursor-pointer font-semibold hover:opacity-80 active:-translate-y-px transition-all rounded shadow-sm"
                  title="Reset to default"
                >
                  <i class="fa-solid fa-rotate-left" />
                </button>
              </div>
              <div class="text-sm text-slate-500 dark:text-slate-400 text-right font-medium">
                {store?.speed}%
              </div>
            </div>
          </SidebarSection>
        </Show>

        <Show when={store?.plugin === 1 && !store?.isActiveScheduler}>
          <div class="my-6 border-t border-slate-200 dark:border-slate-700 hidden lg:block" />

          <div class="hidden lg:block">
            <SidebarSection title="Matrix Controls">
              <div class="grid grid-cols-2 gap-2">
                <button
                  type="button"
                  onClick={props.onLoadImage}
                  class="w-full bg-slate-800 dark:bg-slate-700 text-white border-0 px-3 py-2 text-sm cursor-pointer font-semibold hover:opacity-80 active:-translate-y-px transition-all rounded flex flex-col items-center gap-1 shadow-sm"
                >
                  <i class="fa-solid fa-file-import text-base" />
                  <span class="text-xs">Import</span>
                </button>
                <button
                  type="button"
                  onClick={props.onClear}
                  class="w-full bg-slate-800 dark:bg-slate-700 text-white border-0 px-3 py-2 text-sm cursor-pointer font-semibold hover:opacity-80 active:-translate-y-px transition-all rounded hover:bg-red-600 flex flex-col items-center gap-1 shadow-sm"
                >
                  <i class="fa-solid fa-trash text-base" />
                  <span class="text-xs">Clear</span>
                </button>
                <button
                  type="button"
                  onClick={props.onPersist}
                  class="w-full bg-slate-800 dark:bg-slate-700 text-white border-0 px-3 py-2 text-sm cursor-pointer font-semibold hover:opacity-80 active:-translate-y-px transition-all rounded flex flex-col items-center gap-1 shadow-sm"
                >
                  <i class="fa-solid fa-floppy-disk text-base" />
                  <span class="text-xs">Save</span>
                </button>
                <button
                  type="button"
                  onClick={props.onLoad}
                  class="w-full bg-slate-800 dark:bg-slate-700 text-white border-0 px-3 py-2 text-sm cursor-pointer font-semibold hover:opacity-80 active:-translate-y-px transition-all rounded flex flex-col items-center gap-1 shadow-sm"
                >
                  <i class="fa-solid fa-refresh text-base" />
                  <span class="text-xs">Load</span>
                </button>
              </div>
            </SidebarSection>
          </div>
        </Show>
      </div>

      <div class="flex flex-col shrink-0 pt-6 border-t border-slate-200 dark:border-slate-700 space-y-6">
        <Show when={store?.plugins.some((p) => p.name.includes("Animation"))}>
          <a
            href={getHref("/creator")}
            class={`inline-flex items-center font-medium transition-colors ${currentHash() === "#/creator" ? "text-slate-900 dark:text-white" : "text-slate-600 dark:text-slate-400 hover:text-slate-900 dark:hover:text-white"}`}
          >
            <i class="fa-solid fa-pencil mr-2" />
            Animation Creator
          </a>
        </Show>

        <a
          href={getHref("/scheduler")}
          class={`inline-flex items-center font-medium transition-colors ${currentHash() === "#/scheduler" ? "text-slate-900 dark:text-white" : "text-slate-600 dark:text-slate-400 hover:text-slate-900 dark:hover:text-white"}`}
        >
          <i class="fa-regular fa-clock mr-2" />
          Plugin Scheduler ({store.schedule.length})
        </a>

        <a
          href={getHref("/settings")}
          class={`inline-flex items-center font-medium transition-colors ${currentHash() === "#/settings" ? "text-slate-900 dark:text-white" : "text-slate-600 dark:text-slate-400 hover:text-slate-900 dark:hover:text-white"}`}
        >
          <i class="fa-solid fa-gear mr-2" />
          Settings
        </a>

        <a
          href={getHref("/diagnostics")}
          class={`inline-flex items-center font-medium transition-colors ${currentHash() === "#/diagnostics" ? "text-slate-900 dark:text-white" : "text-slate-600 dark:text-slate-400 hover:text-slate-900 dark:hover:text-white"}`}
        >
          <i class="fa-solid fa-stethoscope mr-2" />
          Diagnostics
        </a>

        <div class="flex items-center justify-between">
          <a
            href="/update"
            class="inline-flex items-center text-slate-600 dark:text-slate-400 hover:text-slate-900 dark:hover:text-white font-medium transition-colors"
          >
            <i class="fa-solid fa-download mr-2" />
            Firmware Update
          </a>

          <button
            type="button"
            onClick={toggleTheme}
            class="text-slate-500 dark:text-slate-400 hover:text-slate-900 dark:hover:text-white transition-colors cursor-pointer"
            title="Toggle Dark Mode"
          >
            <i class={`fa-solid ${isDark() ? "fa-sun" : "fa-moon"} text-lg`} />
          </button>
        </div>
      </div>
    </>
  );
};

export default Sidebar;
