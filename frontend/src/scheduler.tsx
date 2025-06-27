import { Component, For, Index, Show } from 'solid-js';
import Button from './components/button';
import { Layout } from './components/layout/layout';
import { Tooltip } from './components/tooltip';
import { useStore } from './contexts/store';
import { useToast } from './contexts/toast';

const API_URL = import.meta.env.PROD
  ? `http://${window.location.host}/`
  : import.meta.env.VITE_BASE_URL;

export const ResetScheduleButton = () => {
  const { toast } = useToast();

  return (
    <button
      onClick={async () => {
        try {
          const response = await fetch(`${API_URL}api/schedule/clear`);

          if (response.ok) {
            toast('Reset schedule successfully', 2000);
          }
        } catch {
          toast('Failed to reset schedule', 2000);
        }
      }}
      class="w-full bg-blue-600 hover:bg-red-600 text-white border-0 px-4 py-3 uppercase text-sm leading-6 tracking-wider cursor-pointer font-bold hover:opacity-80 active:translate-y-[-1px] transition-all rounded"
    >
      Reset Scheduler
    </button>
  );
};

export const ToggleScheduleButton = () => {
  const [store] = useStore();
  const { toast } = useToast();

  return (
    <>
      <button
        onClick={async () => {
          if (store.isActiveScheduler) {
            try {
              const response = await fetch(`${API_URL}api/schedule/stop`);
              if (response.ok) toast('Stopped schedule successfully', 2000);
            } catch {
              toast('Failed to stop schedule', 2000);
            }
          } else {
            try {
              // Ensure the request is sent as JSON
              const response = await fetch(`${API_URL}api/schedule`, {
                method: 'POST',
                headers: {
                  'Content-Type': 'application/json',
                },
                body: JSON.stringify(store.schedule),
              });

              if (response.ok) {
                toast('Schedule started successfully', 2000);
              } else {
                toast('Failed to start schedule', 2000);
              }
            } catch (error) {
              console.error('Failed to start schedule:', error);
              toast('Failed to start schedule', 2000);
            }
          }
        }}
        class="w-full bg-blue-600 text-white border-0 px-4 py-3 uppercase text-sm leading-6 tracking-wider cursor-pointer font-bold hover:opacity-80 active:translate-y-[-1px] transition-all rounded"
      >
        {store.isActiveScheduler ? 'Stop' : 'Start'} Scheduler
      </button>
    </>
  );
};

const Scheduler: Component = () => {
  const [store, actions] = useStore();
  const { toast } = useToast();

  const handleAddItem = () => {
    actions.setSchedule([
      ...store.schedule,
      {
        pluginId: store.plugins[0].id || 1,
        startTime: '08:00',
        endTime: '17:00',
        brightness: 128,
      },
    ]);
  };

  const handleRemoveItem = (index: number) => {
    actions.setSchedule(store.schedule.filter((_, i) => i !== index));
  };

  const handleItemChange = (index: number, field: string, value: any) => {
    actions.setSchedule(
      store.schedule.map((item, i) =>
        i === index ? { ...item, [field]: value } : item,
      ),
    );
  };

  return (
    <Layout
      content={
        <div class="space-y-3 p-5">
          <h3 class="text-4xl text-white tracking-wide">Scheduler</h3>

          <div class="bg-white p-6 rounded-md">
            <div class="space-y-2">
              <Show
                when={store.schedule?.length > 0}
                fallback={
                  <div class="text-md text-gray-500 italic">
                    No schedule set
                  </div>
                }
              >
                <Index each={store.schedule}>
                  {(item, index) => (
                    <div
                      class={`flex flex-col md:flex-row items-center gap-4 p-4 rounded-lg shadow-sm border hover:border-gray-200 transition-all duration-200 ${
                        store.activeScheduleIndex === index
                          ? 'bg-green-300 border-green-500'
                          : 'bg-white border-gray-100'
                      }`}
                    >
                      <Show
                        when={store.isActiveScheduler}
                        fallback={
                          <button
                            onClick={() => handleRemoveItem(index)}
                            class="p-2 text-gray-500 hover:text-red-500 hover:bg-red-50 rounded-lg transition-all duration-200"
                            aria-label="Remove item"
                          >
                            <i class="fas fa-trash-alt text-lg" />
                          </button>
                        }
                      >
                        <div
                          class="w-4 h-4 rounded-full"
                          classList={{
                            'bg-green-500 animate-pulse': store.activeScheduleIndex === index,
                          }}
                        ></div>
                      </Show>

                      <div class="flex-1 w-full">
                        <select
                          value={item().pluginId}
                          onChange={(e) =>
                            handleItemChange(
                              index,
                              'pluginId',
                              parseInt(e.currentTarget.value),
                            )
                          }
                          class="w-full px-3 py-2 bg-gray-50 border border-gray-200 rounded-lg focus:ring-2 focus:ring-blue-500 focus:border-blue-500 outline-none transition-all duration-200"
                          disabled={store.isActiveScheduler}
                        >
                          <For each={store.plugins}>
                            {(plugin) => (
                              <option value={plugin.id}>{plugin.name}</option>
                            )}
                          </For>
                        </select>
                      </div>
                      <div class="flex items-center gap-2">
                        <input
                          type="time"
                          value={item().startTime}
                          onInput={(e) =>
                            handleItemChange(index, 'startTime', e.currentTarget.value)
                          }
                          class="p-2 border rounded-lg bg-gray-50"
                          disabled={store.isActiveScheduler}
                        />
                        <span>-</span>
                        <input
                          type="time"
                          value={item().endTime}
                          onInput={(e) =>
                            handleItemChange(index, 'endTime', e.currentTarget.value)
                          }
                          class="p-2 border rounded-lg bg-gray-50"
                          disabled={store.isActiveScheduler}
                        />
                      </div>
                      <div class="flex items-center gap-2 w-full md:w-48">
                        <i class="fa-solid fa-sun text-yellow-500"></i>
                        <input
                          type="range"
                          min="-1"
                          max="255"
                          value={item().brightness}
                          onInput={(e) =>
                            handleItemChange(
                              index,
                              'brightness',
                              parseInt(e.currentTarget.value),
                            )
                          }
                          class="w-full"
                          disabled={store.isActiveScheduler}
                        />
                        <span>
                          {item().brightness === -1
                            ? 'Auto'
                            : `${Math.round((item().brightness / 255) * 100)}%`}
                        </span>
                      </div>
                    </div>
                  )}
                </Index>
              </Show>
            </div>
          </div>
        </div>
      }
      sidebar={
        <>
          <div class="space-y-6">
            <div class="space-y-3">
              <h3 class="text-sm font-semibold text-gray-700 uppercase tracking-wide">
                Controls
              </h3>

              <div class="flex flex-col gap-2">
                <Button
                  onClick={handleAddItem}
                  class="hover:bg-green-600 transition-colors"
                >
                  <i class="fa-solid fa-plus mr-2" />
                  Add Item
                </Button>

                <Show when={store.schedule.length > 0}>
                  <div class="my-6 border-t border-gray-200" />
                  <ToggleScheduleButton />
                </Show>

                <Show when={store.isActiveScheduler}>
                  <div class="my-6 border-t border-gray-200" />
                  <Tooltip text="Removes all items and resets state">
                    <ResetScheduleButton />
                  </Tooltip>
                </Show>
              </div>
            </div>
          </div>
          <div class="mt-auto pt-6 border-t border-gray-200">
            <Tooltip text="Return to main editor">
              <a
                href="#/"
                class="inline-flex items-center text-gray-700 hover:text-gray-900 font-medium"
              >
                <i class="fa-solid fa-arrow-left mr-2" />
                Back
              </a>
            </Tooltip>
          </div>
        </>
      }
    />
  );
};

export default Scheduler;