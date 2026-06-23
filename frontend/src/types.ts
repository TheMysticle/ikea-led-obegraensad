export enum SYSTEM_STATUS {
  NONE = "draw",
  WSBINARY = "wsbinary",
  // SYSTEM
  UPDATE = "update",
  LOADING = "loading",
}

export interface ScheduleItem {
  pluginId: number;
  startTime: string; // "HH:MM" format
  endTime: string; // "HH:MM" format
  brightness: number; // 0-255, or -1 for 'don't change'
}

export interface DiagnosticsInfo {
  heap: number;
  uptime: number;
  wifi_rssi: number;
  loggingEnabled?: boolean;
}

export interface StoreActions {
  setIsActiveScheduler: (isActive: boolean) => void;
  setRotation: (rotation: number) => void;
  setPlugins: (plugins: []) => void;
  setPlugin: (plugin: number) => void;
  setBrightness: (brightness: number) => void;
  setIndexMatrix: (indexMatrix: number[]) => void;
  setLeds: (leds: number[]) => void;
  setSystemStatus: (systemStatus: SYSTEM_STATUS) => void;
  setSchedule: (items: ScheduleItem[]) => void;
  setArtnetUniverse: (artnetUniverse: number) => void;
  setGOLDelay: (GOLDelay: number) => void;
  setSpeed: (speed: number) => void;
  setPower: (power: boolean) => void;
  send: (message: string | ArrayBuffer) => void;
  addLog: (log: string) => void;
  setDiagnostics: (info: DiagnosticsInfo) => void;
  clearLogs: () => void;
  setSpotifyVisualizer: (enabled: boolean) => void;
  setBigClockShowSpotify: (enabled: boolean) => void;
  setBigClockShowProgress: (enabled: boolean) => void;
  setBigClockProgressFadeDelay: (delay: number) => void;
}

export interface Store {
  isActiveScheduler: boolean;
  activeScheduleIndex: number; // Add this
  power: boolean;
  rotation: number;
  brightness: number;
  indexMatrix: number[];
  leds: number[];
  plugins: { id: number; name: string }[];
  plugin: number;
  artnetUniverse: number;
  GOLDelay: number;
  speed: number;
  hasSpeedControl: boolean;
  defaultSpeed: number;
  systemStatus: SYSTEM_STATUS;
  connectionState: () => number;
  connectionStatus?: string;
  schedule: ScheduleItem[];
  logs: string[];
  diagnostics: DiagnosticsInfo | null;
  spotifyVisualizer: boolean;
  bigClockShowSpotify: boolean;
  bigClockShowProgress: boolean;
  bigClockProgressFadeDelay: number;
}

export interface IToastContext {
  toast: (text: string, duration: number) => void;
}
