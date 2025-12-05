/**
 * useSensorData Hook
 * Manages sensor data fetching and auto-refresh
 */

import { useState, useEffect } from 'react';
import sensorService from '../services/sensorService';
import type { SensorData } from '../types';

interface UseSensorDataProps {
  tokenVerify: string;
  virtualPin: number;
  limit?: number;
  refreshInterval?: number;
}

interface UseSensorDataReturn {
  data: SensorData[];
  loading: boolean;
  error: string | null;
  fetchSensorData: () => Promise<void>;
}

export const useSensorData = ({
  tokenVerify,
  virtualPin,
  limit = 10,
  refreshInterval = 20000,
}: UseSensorDataProps): UseSensorDataReturn => {
  const [data, setData] = useState<SensorData[]>([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);

  const fetchSensorData = async (): Promise<void> => {
    try {
      const sensorData = await sensorService.getSensorData(
        tokenVerify,
        virtualPin,
        limit
      );
      setData(sensorData);
      setError(null);
    } catch (err) {
      const errorMessage = err instanceof Error 
        ? err.message 
        : 'Lỗi khi tải dữ liệu';
      setError(errorMessage);
    } finally {
      setLoading(false);
    }
  };

  // Initial load
  useEffect(() => {
    fetchSensorData();
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [tokenVerify, virtualPin]);

  // Auto refresh
  useEffect(() => {
    const interval = setInterval(() => {
      fetchSensorData();
    }, refreshInterval);

    return () => clearInterval(interval);
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [tokenVerify, virtualPin, refreshInterval]);

  return {
    data,
    loading,
    error,
    fetchSensorData,
  };
};

export default useSensorData;

