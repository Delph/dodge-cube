type Effect = () => void;
let activeEffect: Effect | null = null;

export class Dep {
  subscribers: Set<Effect>;

  constructor() {
    this.subscribers = new Set();
  }

  depend() {
    if (activeEffect) {
      this.subscribers.add(activeEffect);
    }
  }

  notify() {
    this.subscribers.forEach(sub => sub());
  }
}


export type Ref<T> = {value: T};


function reactive<T extends object>(object: T): Ref<T> {
  const deps = new Map();

  const makeReactive = (object: T) => {
    return new Proxy(object, {
      get(target: T, key: string | symbol): T[keyof T] | undefined | {value: T[keyof T] | undefined} {
        if (!deps.has(key)) {
          deps.set(key, new Dep());
        }

        const dep = deps.get(key);
        dep.depend();
        const value = target[key as keyof T];
        if (typeof value === 'object' && value !== null) {
          return reactive(value);
        }
        return value;
      },
      set(target: T, key: string | symbol, value: T[keyof T]) {
        target[key as keyof T] = value;
        if (deps.has(key)) {
          const dep = deps.get(key);
          dep.notify();
        }
        return true;
      }
    });
  };

  return {value: makeReactive(object)};
}


export function watchEffect(effect: Effect) {
  activeEffect = effect;
  effect();
  activeEffect = null;
}


export function ref<T>(initialValue: T): Ref<T> {
  if (typeof initialValue === 'object' && initialValue !== null) {
    console.log(`ref(${initialValue}) - initialValue is object, not null`);
    return reactive(initialValue);
  }
  console.log(`ref(${initialValue}) - making dep`);
  const dep = new Dep();
  let value = initialValue;

  return {
    get value(): T {
      dep.depend();
      return value;
    },

    set value(newValue: T) {
      if (newValue !== value) {
        value = newValue;
        dep.notify();
      }
    }
  };
}

export function computed<T>(compute: () => T) {
  let value: T;
  let dirty = true;

  const effect = () => {
    if (dirty) {
      value = compute();
      dirty = false;
    }
    return value;
  }

  return {
    get value() {
      dirty = true;
      return effect();
    }
  }
}
