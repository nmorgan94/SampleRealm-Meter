import { createRoot } from 'react-dom/client';
import { Editor } from '../Editor.tsx';
import { meterBackend } from './meterBackend.ts';
import './styles.css';

const root = createRoot(document.getElementById('root')!);

try {
  root.render(<Editor backend={meterBackend()} />);
} catch (error) {
  console.error(error);
  root.render(<pre className="p-5 text-fg">{`The editor couldn't connect to the plugin.\n${error}`}</pre>);
}
