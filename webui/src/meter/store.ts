import { listenerSet } from '@samplerealm/modules/backend.ts';

// A value a backend holds beside the parameters, and those listening for it
export function valueStore<T>(initial: T) {
  let value = initial;
  const listeners = listenerSet<[T]>();

  return {
    get: () => value,
    listen: listeners.add,
    show: (next: T) => {
      value = next;
      listeners.notify(next);
    },
  };
}
