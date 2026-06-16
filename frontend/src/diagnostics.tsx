import { type Component, createSignal, For, onCleanup, onMount } from "solid-js";

import { Layout } from "./components/layout/layout";
import Sidebar from "./components/layout/sidebar";
import { useStore } from "./contexts/store";

export const Diagnostics: Component = () => {
  const [store, actions] = useStore();
  let logsContainerRef: HTMLDivElement | undefined;

  onMount(() => {
    // Request diagnostics initially
    actions.send(JSON.stringify({ event: "diagnostics" }));

    // Poll diagnostics every 5 seconds
    const interval = setInterval(() => {
      actions.send(JSON.stringify({ event: "diagnostics" }));
    }, 5000);

    onCleanup(() => {
      clearInterval(interval);
      // Disable logging when leaving page
      actions.send(JSON.stringify({ event: "enable-logging", enabled: false }));
    });
  });

  const toggleLogging = (e: Event) => {
    const checked = (e.target as HTMLInputElement).checked;
    actions.send(JSON.stringify({ event: "enable-logging", enabled: checked }));
  };

  const clearLogs = () => {
    actions.clearLogs();
  };

  const handleRotate = (turnRight = false) => {
    const currentRotation = store.rotation || 0;
    actions.setRotation((currentRotation + (turnRight ? 1 : -1) + 4) % 4);
    actions.send(
      JSON.stringify({
        event: "rotate",
        direction: turnRight ? "right" : "left",
      }),
    );
  };

  const wsMessage = (event: string, data?: any) => actions.send(JSON.stringify({ event, ...data }));

  const renderContent = () => (
    <div class="p-8 h-full overflow-y-auto bg-slate-50 dark:bg-slate-900 text-slate-800 dark:text-slate-200">
      <div class="max-w-4xl mx-auto space-y-6">
        <h1 class="text-3xl font-bold text-slate-900 dark:text-slate-100">Diagnostics</h1>

        {/* System Metrics */}
        <div class="grid grid-cols-1 md:grid-cols-3 gap-4">
          <div class="bg-white dark:bg-slate-800 p-6 rounded-lg shadow-sm border border-slate-200 dark:border-slate-700">
            <div class="text-sm font-medium text-slate-500 dark:text-slate-400 mb-1">Free Heap</div>
            <div class="text-2xl font-bold text-slate-900 dark:text-slate-100">
              {store.diagnostics
                ? `${(store.diagnostics.heap / 1024).toFixed(2)} KB`
                : "Loading..."}
            </div>
          </div>
          <div class="bg-white dark:bg-slate-800 p-6 rounded-lg shadow-sm border border-slate-200 dark:border-slate-700">
            <div class="text-sm font-medium text-slate-500 dark:text-slate-400 mb-1">Uptime</div>
            <div class="text-2xl font-bold text-slate-900 dark:text-slate-100">
              {store.diagnostics
                ? `${Math.floor(store.diagnostics.uptime / 60)}m ${store.diagnostics.uptime % 60}s`
                : "Loading..."}
            </div>
          </div>
          <div class="bg-white dark:bg-slate-800 p-6 rounded-lg shadow-sm border border-slate-200 dark:border-slate-700">
            <div class="text-sm font-medium text-slate-500 dark:text-slate-400 mb-1">WiFi RSSI</div>
            <div class="text-2xl font-bold text-slate-900 dark:text-slate-100">
              {store.diagnostics ? `${store.diagnostics.wifi_rssi} dBm` : "Loading..."}
            </div>
          </div>
        </div>

        {/* Live Logging */}
        <div class="bg-white dark:bg-slate-800 p-6 rounded-lg shadow-sm border border-slate-200 dark:border-slate-700 flex flex-col h-[500px]">
          <div class="flex items-center justify-between mb-4">
            <h2 class="text-xl font-semibold text-slate-900 dark:text-slate-100">Live Logging</h2>
            <div class="flex items-center gap-4">
              <button
                type="button"
                onClick={clearLogs}
                class="px-3 py-1 text-sm bg-slate-200 dark:bg-slate-700 hover:bg-slate-300 dark:hover:bg-slate-600 rounded transition-colors"
              >
                Clear
              </button>
              <div class="flex items-center gap-2">
                <input
                  type="checkbox"
                  id="enableLogging"
                  class="w-4 h-4 text-slate-900 bg-white dark:bg-slate-900 border-slate-300 dark:border-slate-600 rounded focus:ring-slate-500"
                  checked={store.diagnostics?.loggingEnabled ?? false}
                  onChange={toggleLogging}
                />
                <label for="enableLogging" class="text-sm font-medium cursor-pointer">
                  Enable Web Stream
                </label>
              </div>
            </div>
          </div>

          <div
            ref={logsContainerRef}
            class="flex-1 bg-slate-900 dark:bg-black rounded border border-slate-700 overflow-y-auto p-4 font-mono text-sm text-green-400 whitespace-pre-wrap break-all"
          >
            {store.logs.length === 0 && (
              <span class="text-slate-500 italic">No logs received yet...</span>
            )}
            <For each={store.logs}>{(log) => <div>{log}</div>}</For>
          </div>
        </div>
      </div>
    </div>
  );

  return (
    <Layout
      content={renderContent()}
      sidebar={
        <Sidebar
          onRotate={handleRotate}
          onLoadImage={() => {}}
          onClear={() => {}}
          onPersist={() => wsMessage("persist")}
          onLoad={() => wsMessage("load")}
          onPluginChange={(pluginId) => wsMessage("plugin", { plugin: pluginId })}
          onBrightnessChange={(value, shouldSend) => {
            actions.setBrightness(value);
            if (shouldSend) wsMessage("brightness", { brightness: value });
          }}
          onArtnetChange={(value, shouldSend) => {
            actions.setArtnetUniverse(value);
            if (shouldSend) wsMessage("artnet", { universe: value });
          }}
          onSpeedChange={(value, shouldSend) => {
            actions.setSpeed(value);
            if (shouldSend) wsMessage("speed", { speed: value });
          }}
          onPersistPlugin={() => wsMessage("persist-plugin")}
        />
      }
    />
  );
};

export default Diagnostics;
