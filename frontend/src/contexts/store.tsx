import { createEventSignal } from "@solid-primitives/event-listener";
import { createReconnectingWS, createWSState } from "@solid-primitives/websocket";
import { batch, createContext, createEffect, type JSX, useContext } from "solid-js";
import { createStore } from "solid-js/store";

import { type ScheduleItem, type Store, type StoreActions, SYSTEM_STATUS } from "../types";
import { ToastProvider } from "./toast";

const ws = createReconnectingWS(
  `${
    import.meta.env.PROD
      ? `${window.location.protocol === "https:" ? "wss:" : "ws:"}//${window.location.host}/`
      : import.meta.env.VITE_WS_URL
  }ws`,
);

const wsState = createWSState(ws);

const connectionStatus = ["Connecting", "Connected", "Disconnecting", "Disconnected"];

const [mainStore, setStore] = createStore<Store>({
  isActiveScheduler: false,
  activeScheduleIndex: -1, // Add this
  power: true,
  rotation: 0,
  plugins: [],
  plugin: 1,
  brightness: 0,
  artnetUniverse: 1,
  GOLDelay: 150,
  indexMatrix: [...new Array(256)].map((_, i) => i),
  leds: [...new Array(256)].fill(0),
  systemStatus: SYSTEM_STATUS.NONE,
  connectionState: wsState,
  connectionStatus: connectionStatus[0],
  schedule: [],
  logs: [],
  diagnostics: null,
});

const actions: StoreActions = {
  setIsActiveScheduler: (isActive) => setStore("isActiveScheduler", isActive),
  setActiveScheduleIndex: (index) => setStore("activeScheduleIndex", index), // Add this
  setPower: (power) => setStore("power", power),
  setRotation: (rotation) => setStore("rotation", rotation),
  setPlugins: (plugins) => setStore("plugins", plugins),
  setPlugin: (plugin) => setStore("plugin", plugin),
  setBrightness: (brightness) => setStore("brightness", brightness),
  setArtnetUniverse: (artnetUniverse) => setStore("artnetUniverse", artnetUniverse),
  setSpeed: (speed) => setStore("speed", speed),
  setIndexMatrix: (indexMatrix) => setStore("indexMatrix", indexMatrix),
  setLeds: (leds) => setStore("leds", leds),
  setSystemStatus: (systemStatus: SYSTEM_STATUS) => setStore("systemStatus", systemStatus),
  setSchedule: (items: ScheduleItem[]) => setStore("schedule", items),
  send: ws.send,
  addLog: (log: string) => setStore("logs", (prev) => [...prev, log].slice(-100)), // keep last 100 logs
  setDiagnostics: (info: DiagnosticsInfo) => setStore("diagnostics", info),
  clearLogs: () => setStore("logs", []),
};

const store: [Store, StoreActions] = [mainStore, actions] as const;

const StoreContext = createContext<[Store, StoreActions]>(store);

const isValidNumber = (value: unknown): value is number =>
  typeof value === "number" && !Number.isNaN(value);

const isValidBoolean = (value: unknown): value is boolean => typeof value === "boolean";

const isValidArray = (value: unknown): value is unknown[] => Array.isArray(value);

export const StoreProvider = (props?: { value?: Store; children?: JSX.Element }) => {
  const messageEvent = createEventSignal<{ message: MessageEvent }>(ws, "message");
  const errorEvent = createEventSignal<{ error: Event }>(ws, "error");

  createEffect(() => {
    const state = wsState();

    if (state >= 0 && state < connectionStatus.length) {
      setStore("connectionStatus", connectionStatus[state]);
    }

    if (state === WebSocket.CLOSED || state === WebSocket.CLOSING) {
      console.warn("WebSocket disconnected. Will attempt to reconnect...");
    }
  });

  createEffect(() => {
    const error = errorEvent();
    if (error) {
      console.error("WebSocket error occurred:", error);
    }
  });

  createEffect(() => {
    try {
      const json = JSON.parse(messageEvent()?.data || "{}");

      if (!json.event || typeof json.event !== "string") {
        return;
      }

      switch (json.event) {
        case "minimal-info":
          batch(() => {
            if (
              isValidNumber(json.status) &&
              json.status >= 0 &&
              json.status < Object.values(SYSTEM_STATUS).length
            ) {
              actions.setSystemStatus(Object.values(SYSTEM_STATUS)[json.status]);
            }
            if (isValidNumber(json.rotation)) actions.setRotation(json.rotation);
            if (isValidNumber(json.brightness)) actions.setBrightness(json.brightness);
            if (isValidBoolean(json.power)) actions.setPower(json.power);
            if (isValidNumber(json.plugin)) actions.setPlugin(json.plugin);
            if (isValidBoolean(json.scheduleActive))
              actions.setIsActiveScheduler(json.scheduleActive);
            if (isValidNumber(json.activeScheduleIndex))
              actions.setActiveScheduleIndex(json.activeScheduleIndex);

            setStore(
              "hasSpeedControl",
              isValidBoolean(json.hasSpeedControl) ? json.hasSpeedControl : false,
            );
            if (isValidNumber(json.speed)) setStore("speed", json.speed);
            if (isValidNumber(json.defaultSpeed)) setStore("defaultSpeed", json.defaultSpeed);
          });
          break;
        case "info":
          batch(() => {
            if (
              isValidNumber(json.status) &&
              json.status >= 0 &&
              json.status < Object.values(SYSTEM_STATUS).length
            ) {
              actions.setSystemStatus(Object.values(SYSTEM_STATUS)[json.status]);
            }

            if (isValidNumber(json.rotation)) {
              actions.setRotation(json.rotation);
            }

            if (isValidNumber(json.brightness)) {
              actions.setBrightness(json.brightness);
            }

            if (isValidBoolean(json.power)) {
              actions.setPower(json.power);
            }

            if (isValidBoolean(json.scheduleActive)) {
              actions.setIsActiveScheduler(json.scheduleActive);
            }

            if (isValidArray(json.schedule)) {
              actions.setSchedule(json.schedule as ScheduleItem[]);
            }

            if (isValidNumber(json.activeScheduleIndex)) {
              actions.setActiveScheduleIndex(json.activeScheduleIndex);
            }

            if (!mainStore.plugins.length && isValidArray(json.plugins)) {
              actions.setPlugins(json.plugins);
            }

            if (isValidNumber(json.plugin)) {
              actions.setPlugin(json.plugin);
            }

            if (mainStore.plugin === 1) {
              actions.setIndexMatrix([...new Array(256)].map((_, i) => i));
            }

            setStore(
              "hasSpeedControl",
              isValidBoolean(json.hasSpeedControl) ? json.hasSpeedControl : false,
            );
            if (isValidNumber(json.speed)) setStore("speed", json.speed);
            if (isValidNumber(json.defaultSpeed)) setStore("defaultSpeed", json.defaultSpeed);

            if (isValidArray(json.data)) {
              actions.setLeds(json.data as number[]);
            }
          });
          break;
        case "log":
          if (json.message) actions.addLog(json.message);
          break;
        case "diagnostics":
          if (
            isValidNumber(json.heap) &&
            isValidNumber(json.uptime) &&
            isValidNumber(json.wifi_rssi)
          ) {
            actions.setDiagnostics({
              heap: json.heap,
              uptime: json.uptime,
              wifi_rssi: json.wifi_rssi,
              loggingEnabled: json.loggingEnabled === true,
            });
          }
          break;
      }
    } catch (error) {
      console.error("Failed to parse WebSocket message:", error);
    }
  });

  return (
    <ToastProvider>
      <StoreContext.Provider value={store}>{props?.children}</StoreContext.Provider>
    </ToastProvider>
  );
};

export const useStore = () => {
  if (StoreContext) return useContext(StoreContext);

  return [{}, {}] as [Store, StoreActions];
};
