/**
 * useDeviceConfig Hook
 * Manages device configuration pins
 */

import { useState, useEffect } from 'react';
import deviceService from '../services/deviceService';
import type { ConfigPin } from '../types';

interface UseDeviceConfigProps {
  deviceToken: string | null | undefined;
}

interface UseDeviceConfigReturn {
  configPins: ConfigPin[];
  loading: boolean;
  error: string | null;
  loadConfigPins: () => Promise<void>;
  addPin: (pin: Omit<ConfigPin, 'device_token'>) => Promise<void>;
  deletePin: (virtualPin: number) => Promise<void>;
}

export const useDeviceConfig = ({
  deviceToken,
}: UseDeviceConfigProps): UseDeviceConfigReturn => {
  const [configPins, setConfigPins] = useState<ConfigPin[]>([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);

  const loadConfigPins = async () => {
    if (!deviceToken) {
      setError('Không tìm thấy device token');
      setLoading(false);
      return;
    }

    setLoading(true);
    setError(null);

    try {
      const pins = await deviceService.getConfigPins(deviceToken);
      setConfigPins(pins);
    } catch (err) {
      const errorMessage = err instanceof Error 
        ? err.message 
        : 'Lỗi khi tải danh sách virtual pins';
      setError(errorMessage);
      setConfigPins([]);
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    loadConfigPins();
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [deviceToken]);

  const addPin = async (pin: Omit<ConfigPin, 'device_token'>): Promise<void> => {
    if (!deviceToken) {
      throw new Error('Không tìm thấy device token');
    }

    // Check if virtual_pin already exists
    if (configPins.some((p) => p.virtual_pin === pin.virtual_pin)) {
      throw new Error(`Virtual pin ${pin.virtual_pin} đã được sử dụng`);
    }

    // Create array with all existing pins + new pin
    const allPins: ConfigPin[] = [
      ...configPins.map((p) => ({
        virtual_pin: p.virtual_pin,
        pin_label: p.pin_label,
        pin_type: p.pin_type,
        data_type: p.data_type,
        ai_keywords: p.ai_keywords || '',
      })),
      {
        ...pin,
        ai_keywords: pin.pin_type === 'OUTPUT' ? pin.ai_keywords || '' : '',
      },
    ];

    await deviceService.saveConfigPins({
      device_token: deviceToken,
      pins: allPins,
    });

    await loadConfigPins();
  };

  const deletePin = async (virtualPin: number): Promise<void> => {
    if (!deviceToken) {
      throw new Error('Không tìm thấy device token');
    }

    if (!window.confirm(`Bạn có chắc chắn muốn xóa virtual pin ${virtualPin}?`)) {
      return;
    }

    await deviceService.deleteConfigPin(deviceToken, virtualPin);
    await loadConfigPins();
    alert(`Đã xóa virtual pin ${virtualPin} thành công`);
  };

  return {
    configPins,
    loading,
    error,
    loadConfigPins,
    addPin,
    deletePin,
  };
};

export default useDeviceConfig;

