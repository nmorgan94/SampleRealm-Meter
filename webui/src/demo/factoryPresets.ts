// The plugin's own factory presets
const files = import.meta.glob<string>('../../../Assets/Presets/*.srmeter', { query: '?raw', import: 'default', eager: true });

export const factoryPresets = Object.values(files);
