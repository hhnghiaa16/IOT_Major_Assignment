/**
 * useDevices Hook
 * Manages device list and operations
 */

import { useState, useEffect, useCallback } from 'react';
import deviceService from '../services/deviceService';
import type { Device } from '../types';

interface UseDevicesReturn {
  devices: Device[];
  loading: boolean;
  error: string | null;
  loadDevices: () => Promise<void>;
}

export const useDevices = (): UseDevicesReturn => {
  const [devices, setDevices] = useState<Device[]>([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);

  const loadDevices = useCallback(async () => {
    setLoading(true);
    setError(null);
    
    try {
      const deviceList = await deviceService.getDevices();
      setDevices(deviceList);
    } catch (err) {
      const errorMessage = err instanceof Error 
        ? err.message 
        : 'Lỗi khi tải danh sách thiết bị';
      setError(errorMessage);
    } finally {
      setLoading(false);
    }
  }, []);

  useEffect(() => {
    loadDevices();
  }, [loadDevices]);

  return {
    devices,
    loading,
    error,
    loadDevices,
  };
};

export default useDevices;

