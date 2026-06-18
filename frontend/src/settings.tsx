import { type Component, createSignal, onMount } from "solid-js";

import { Layout } from "./components/layout/layout";
import Sidebar from "./components/layout/sidebar";
import { useStore } from "./contexts/store";
import { useToast } from "./contexts/toast";
import { rotateArray } from "./helpers";

interface Config {
  weatherLocation: string;
  ntpServer: string;
  tzInfo: string;
  tessieVin: string;
  tessieApiKey: string;
  autoStartSchedule: boolean;
  alexaEnabled: boolean;
  alexaDeviceName: string;
  doublePressPlugin: number;
}

export const Settings: Component = () => {
  const [store, actions] = useStore();
  const { toast } = useToast();

  const [config, setConfig] = createSignal<Config>({
    weatherLocation: "",
    ntpServer: "",
    tzInfo: "",
    tessieVin: "",
    tessieApiKey: "",
    autoStartSchedule: false,
    alexaEnabled: false,
    alexaDeviceName: "LED Wall",
    doublePressPlugin: 19,
  });

  const [loading, setLoading] = createSignal(true);

  const fetchConfig = async () => {
    setLoading(true);
    try {
      const res = await fetch("/api/config");
      if (res.ok) {
        const data = await res.json();
        setConfig(data);
      } else {
        toast("Failed to load settings", 2000);
      }
    } catch (err) {
      toast("Error loading settings", 2000);
    } finally {
      setLoading(false);
    }
  };

  onMount(() => {
    fetchConfig();
  });

  const handleSave = async (e: Event) => {
    e.preventDefault();
    setLoading(true);
    try {
      const res = await fetch("/api/config", {
        method: "POST",
        headers: {
          "Content-Type": "application/json",
        },
        body: JSON.stringify(config()),
      });
      if (res.ok) {
        toast("Settings saved successfully", 2000);
      } else {
        toast("Failed to save settings", 2000);
      }
    } catch (err) {
      toast("Error saving settings", 2000);
    } finally {
      setLoading(false);
    }
  };

  const handleReset = async () => {
    if (!confirm("Are you sure you want to reset all settings to defaults?")) return;
    setLoading(true);
    try {
      const res = await fetch("/api/config/reset", {
        method: "POST",
      });
      if (res.ok) {
        toast("Settings reset to defaults", 2000);
        await fetchConfig();
      } else {
        toast("Failed to reset settings", 2000);
      }
    } catch (err) {
      toast("Error resetting settings", 2000);
    } finally {
      setLoading(false);
    }
  };

  const updateField = (field: keyof Config, value: string | boolean) => {
    setConfig((prev) => ({ ...prev, [field]: value }));
  };

  const renderContent = () => (
    <div class="p-8 h-full overflow-y-auto bg-slate-50 dark:bg-slate-900 text-slate-800 dark:text-slate-200">
      <div class="max-w-2xl mx-auto">
        <h1 class="text-3xl font-bold mb-6 text-slate-900 dark:text-slate-100">Device Settings</h1>

        {loading() ? (
          <div class="flex justify-center items-center py-12">
            <div class="animate-spin rounded-full h-8 w-8 border-b-2 border-slate-900 dark:border-slate-100" />
          </div>
        ) : (
          <form
            onSubmit={handleSave}
            class="space-y-6 bg-white dark:bg-slate-800 p-6 rounded-lg shadow-sm border border-slate-200 dark:border-slate-700"
          >
            <div class="space-y-4">
              <h2 class="text-xl font-semibold border-b border-slate-200 dark:border-slate-700 pb-2">
                General
              </h2>

              <div>
                <label class="block text-sm font-medium text-slate-700 dark:text-slate-300 mb-1">
                  Weather Location (City)
                </label>
                <input
                  type="text"
                  class="w-full px-3 py-2 bg-white dark:bg-slate-900 border border-slate-300 dark:border-slate-600 rounded focus:outline-none focus:ring-2 focus:ring-slate-500 dark:focus:ring-slate-400 dark:text-slate-100"
                  value={config().weatherLocation}
                  onInput={(e) => updateField("weatherLocation", e.currentTarget.value)}
                  placeholder="e.g. London, New York"
                />
              </div>

              <div>
                <label class="block text-sm font-medium text-slate-700 dark:text-slate-300 mb-1">
                  Timezone String
                </label>
                <input
                  type="text"
                  class="w-full px-3 py-2 bg-white dark:bg-slate-900 border border-slate-300 dark:border-slate-600 rounded focus:outline-none focus:ring-2 focus:ring-slate-500 dark:focus:ring-slate-400 dark:text-slate-100"
                  value={config().tzInfo}
                  onInput={(e) => updateField("tzInfo", e.currentTarget.value)}
                  placeholder="e.g. CET-1CEST,M3.5.0,M10.5.0/3"
                />
              </div>

              <div>
                <label class="block text-sm font-medium text-slate-700 dark:text-slate-300 mb-1">
                  NTP Server
                </label>
                <input
                  type="text"
                  class="w-full px-3 py-2 bg-white dark:bg-slate-900 border border-slate-300 dark:border-slate-600 rounded focus:outline-none focus:ring-2 focus:ring-slate-500 dark:focus:ring-slate-400 dark:text-slate-100"
                  value={config().ntpServer}
                  onInput={(e) => updateField("ntpServer", e.currentTarget.value)}
                  placeholder="e.g. pool.ntp.org"
                />
              </div>

              <div class="flex items-center">
                <input
                  type="checkbox"
                  id="autoStartSchedule"
                  class="w-4 h-4 text-slate-600 bg-white dark:bg-slate-900 border-slate-300 dark:border-slate-600 rounded focus:ring-slate-500"
                  checked={config().autoStartSchedule}
                  onChange={(e) => updateField("autoStartSchedule", e.currentTarget.checked)}
                />
                <label
                  for="autoStartSchedule"
                  class="ml-2 block text-sm text-slate-900 dark:text-slate-200"
                >
                  Auto-start Schedule on boot
                </label>
              </div>

              <div>
                <label class="block text-sm font-medium text-slate-700 dark:text-slate-300 mb-1">
                  Double Press Action
                </label>
                <select
                  class="w-full px-3 py-2 bg-white dark:bg-slate-900 border border-slate-300 dark:border-slate-600 rounded focus:outline-none focus:ring-2 focus:ring-slate-500 dark:focus:ring-slate-400 dark:text-slate-100"
                  value={config().doublePressPlugin}
                  onChange={(e) => updateField("doublePressPlugin", parseInt(e.currentTarget.value))}
                >
                  {store.plugins.map((p) => (
                    <option value={p.id}>{p.name}</option>
                  ))}
                </select>
              </div>
            </div>

            <div class="space-y-4 pt-4">
              <h2 class="text-xl font-semibold border-b border-slate-200 dark:border-slate-700 pb-2">
                Tessie Integration
              </h2>
              <p class="text-sm text-slate-500 dark:text-slate-400">
                Provide your Tessie API credentials to display Tesla charging status.
              </p>

              <div>
                <label class="block text-sm font-medium text-slate-700 dark:text-slate-300 mb-1">
                  VIN (Vehicle Identification Number)
                </label>
                <input
                  type="text"
                  class="w-full px-3 py-2 bg-white dark:bg-slate-900 border border-slate-300 dark:border-slate-600 rounded focus:outline-none focus:ring-2 focus:ring-slate-500 dark:focus:ring-slate-400 font-mono text-sm dark:text-slate-100"
                  value={config().tessieVin}
                  onInput={(e) => updateField("tessieVin", e.currentTarget.value)}
                  placeholder="Enter 17-character VIN"
                />
              </div>

              <div>
                <label class="block text-sm font-medium text-slate-700 dark:text-slate-300 mb-1">
                  API Key
                </label>
                <input
                  type="password"
                  class="w-full px-3 py-2 bg-white dark:bg-slate-900 border border-slate-300 dark:border-slate-600 rounded focus:outline-none focus:ring-2 focus:ring-slate-500 dark:focus:ring-slate-400 font-mono text-sm dark:text-slate-100"
                  value={config().tessieApiKey}
                  onInput={(e) => updateField("tessieApiKey", e.currentTarget.value)}
                  placeholder="Enter your Tessie API Token"
                />
              </div>
            </div>

            <div class="space-y-4 pt-4">
              <h2 class="text-xl font-semibold border-b border-slate-200 dark:border-slate-700 pb-2">
                Alexa Integration
              </h2>
              <p class="text-sm text-slate-500 dark:text-slate-400">
                Allow Amazon Alexa to discover and control this device.
              </p>

              <div class="flex items-center">
                <input
                  type="checkbox"
                  id="alexaEnabled"
                  class="w-4 h-4 text-slate-600 bg-white dark:bg-slate-900 border-slate-300 dark:border-slate-600 rounded focus:ring-slate-500"
                  checked={config().alexaEnabled}
                  onChange={(e) => updateField("alexaEnabled", e.currentTarget.checked)}
                />
                <label
                  for="alexaEnabled"
                  class="ml-2 block text-sm text-slate-900 dark:text-slate-200"
                >
                  Enable Alexa Integration
                </label>
              </div>

              <div>
                <label class="block text-sm font-medium text-slate-700 dark:text-slate-300 mb-1">
                  Voice Command Device Name
                </label>
                <input
                  type="text"
                  disabled={!config().alexaEnabled}
                  class="w-full px-3 py-2 bg-white dark:bg-slate-900 border border-slate-300 dark:border-slate-600 rounded focus:outline-none focus:ring-2 focus:ring-slate-500 dark:focus:ring-slate-400 dark:text-slate-100 disabled:opacity-50"
                  value={config().alexaDeviceName}
                  onInput={(e) => updateField("alexaDeviceName", e.currentTarget.value)}
                  placeholder="e.g. LED Wall"
                />
              </div>
            </div>

            <div class="flex items-center justify-between pt-6 border-t border-slate-200 dark:border-slate-700 mt-6">
              <button
                type="button"
                onClick={handleReset}
                class="px-4 py-2 text-sm text-red-600 dark:text-red-400 hover:text-red-800 dark:hover:text-red-300 font-medium transition-colors"
              >
                Reset Defaults
              </button>

              <button
                type="submit"
                disabled={loading()}
                class="px-6 py-2 bg-slate-900 dark:bg-slate-700 text-white font-medium rounded hover:bg-slate-800 dark:hover:bg-slate-600 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-slate-900 dark:focus:ring-slate-500 disabled:opacity-50 transition-colors shadow-sm"
              >
                Save Settings
              </button>
            </div>
          </form>
        )}
      </div>
    </div>
  );

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
          onGOLDelayChange={(value, shouldSend) => {
            actions.setGOLDelay(value);
            if (shouldSend) wsMessage("goldelay", { delay: value });
          }}
          onPersistPlugin={() => wsMessage("persist-plugin")}
        />
      }
    />
  );
};

export default Settings;
