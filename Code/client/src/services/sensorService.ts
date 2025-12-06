/**
 * Sensor Service
 * Handles all sensor data-related API calls
 */

import apiClient from './apiClient';
import type { SensorData, SensorDataResponse } from '../types';

export const sensorService = {
  /**
   * Get sensor data for a specific device and virtual pin
   */
  getSensorData: async (
    tokenVerify: string,
    virtualPin: number,
    limit: number = 10
  ): Promise<SensorData[]> => {
    const response = await apiClient.get<SensorDataResponse>(
      `/sensors/sensor-data?token_verify=${tokenVerify}&limit=${limit}&virtual_pin=${virtualPin}`
    );
    
    if (!response.data) {
      return [];
    }
    
    // Sort by timestamp ascending (oldest to newest) and take last N items
    return response.data
      .sort((a, b) => 
        new Date(a.timestamp).getTime() - new Date(b.timestamp).getTime()
      )
      .slice(-limit);
  },
};

export default sensorService;

