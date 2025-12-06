/**
 * useBlockConfig Hook
 * Manages block configuration modal state and operations
 */

import { useState, useEffect } from 'react';
import dashboardService from '../services/dashboardService';
import deviceService from '../services/deviceService';
import type { DashboardBlock, BlockConfigDevice, BlockConfigPin, Device } from '../types';

interface UseBlockConfigProps {
  block: DashboardBlock | null;
  isNewBlock: boolean;
  blockType: number;
}

interface UseBlockConfigReturn {
  devices: BlockConfigDevice[];
  pins: BlockConfigPin[];
  loading: boolean;
  formData: {
    id: number;
    device_name: string;
    token_verify: string;
    pin_label: string;
    virtual_pin: number;
    label_block: string;
    type_block: number;
  };
  setFormData: React.Dispatch<React.SetStateAction<UseBlockConfigReturn['formData']>>;
  error: string | null;
  saving: boolean;
  handleDeviceChange: (deviceName: string) => Promise<void>;
  handlePinChange: (virtualPin: number) => void;
  handleSubmit: (e: React.FormEvent) => Promise<void>;
}

export const useBlockConfig = ({
  block,
  isNewBlock,
  blockType,
}: UseBlockConfigProps): UseBlockConfigReturn => {
  const [devices, setDevices] = useState<BlockConfigDevice[]>([]);
  const [pins, setPins] = useState<BlockConfigPin[]>([]);
  const [loading, setLoading] = useState(true);
  const [formData, setFormData] = useState({
    id: block?.id || -1,
    device_name: block?.device_name || '',
    token_verify: block?.token_verify || '',
    pin_label: block?.pin_label || '',
    virtual_pin: block?.virtual_pin || 0,
    label_block: block?.label_block || '',
    type_block: isNewBlock ? blockType : block?.typeblock || 0,
  });
  const [error, setError] = useState<string | null>(null);
  const [saving, setSaving] = useState(false);

  useEffect(() => {
    const init = async () => {
      setLoading(true);
      try {
        const deviceList = await deviceService.getDevices();
        // Filter and map devices to BlockConfigDevice format (only devices with device_token and token_verify)
        const validDevices: BlockConfigDevice[] = deviceList
          .filter((d): d is Device & { device_token: string; token_verify: string } => 
            Boolean(d.device_token && d.token_verify)
          )
          .map((d) => ({
            id: d.id,
            device_name: d.device_name,
            device_token: d.device_token!,
            token_verify: d.token_verify!,
          }));
        setDevices(validDevices);

        // If editing existing block, load pins
        if (!isNewBlock && block?.token_verify) {
          const device = validDevices.find((d) => d.token_verify === block.token_verify);
          if (device) {
            const allPins = await deviceService.getConfigPins(device.device_token);
            const filteredPins = allPins.filter((pin) =>
              block.typeblock === 0
                ? pin.pin_type === 'OUTPUT'
                : pin.pin_type === 'INPUT'
            );
            setPins(filteredPins);
          }
        }
      } catch (err) {
        console.error('Error initializing block config:', err);
      } finally {
        setLoading(false);
      }
    };

    init();
  }, [block, isNewBlock]);

  const handleDeviceChange = async (deviceName: string): Promise<void> => {
    const device = devices.find((d) => d.device_name === deviceName);
    if (!device || !device.device_token) return;

    setFormData((prev) => ({
      ...prev,
      device_name: device.device_name,
      token_verify: device.token_verify || '',
      virtual_pin: 0,
      pin_label: '',
    }));

    // Load pins for this device
    const allPins = await deviceService.getConfigPins(device.device_token);
    const filteredPins = allPins.filter((pin) =>
      formData.type_block === 0 ? pin.pin_type === 'OUTPUT' : pin.pin_type === 'INPUT'
    );
    setPins(filteredPins);
  };

  const handlePinChange = (virtualPin: number): void => {
    const pin = pins.find((p) => p.virtual_pin === virtualPin);
    if (!pin) return;

    setFormData((prev) => ({
      ...prev,
      virtual_pin: pin.virtual_pin,
      pin_label: pin.pin_label,
    }));
  };

  const handleSubmit = async (e: React.FormEvent): Promise<void> => {
    e.preventDefault();
    setError(null);

    if (!formData.device_name) {
      setError('Vui lòng chọn thiết bị');
      return;
    }

    if (!formData.label_block.trim()) {
      setError('Vui lòng nhập tên block');
      return;
    }

    if (!formData.token_verify) {
      setError('Token thiết bị không hợp lệ');
      return;
    }

    setSaving(true);
    try {
      await dashboardService.saveBlock(formData);
    } catch (err) {
      const errorMessage = err instanceof Error 
        ? err.message 
        : 'Lỗi khi lưu cấu hình';
      setError(errorMessage);
    } finally {
      setSaving(false);
    }
  };

  return {
    devices,
    pins,
    loading,
    formData,
    setFormData,
    error,
    saving,
    handleDeviceChange,
    handlePinChange,
    handleSubmit,
  };
};

export default useBlockConfig;

