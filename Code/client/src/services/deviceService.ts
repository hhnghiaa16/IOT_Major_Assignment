/**
 * Device Service
 * Handles all device-related API calls
 */

import apiClient from './apiClient';
import type {
  Device,
  DeviceListResponse,
  DeviceInfoResponse,
  RegisterDeviceRequest,
  RegisterDeviceResponse,
  ConfigPin,
  ConfigPinListResponse,
  ConfigPinRequest,
  ConfigPinResponse,
  MasterDevice,
  MasterDeviceListResponse,
  SlaveDevice,
  SlaveDeviceListResponse,
} from '../types';

export const deviceService = {
  /**
   * Get all devices for the current user
   */
  getDevices: async (): Promise<Device[]> => {
    const response = await apiClient.get<DeviceListResponse>('/devices/getDevices');

    if (!response.success) {
      throw new Error(response.message || 'Failed to fetch devices');
    }

    // Normalize connection_status
    return (response.devices || []).map((device) => ({
      ...device,
      connection_status: device.connection_status ||
        (device.is_active ? 'ONLINE' : 'OFFLINE') as 'ONLINE' | 'OFFLINE',
    }));
  },

  /**
   * Get all master devices for the current user
   */
  getMasterDevices: async (): Promise<MasterDevice[]> => {
    const response = await apiClient.get<MasterDeviceListResponse>('/devices/getMasterDevice');

    if (!response.success) {
      throw new Error(response.message || 'Failed to fetch master devices');
    }

    // Normalize connection_status
    return (response.devices || []).map((device) => ({
      ...device,
      connection_status: device.connection_status ||
        (device.is_active ? 'ONLINE' : 'OFFLINE') as 'ONLINE' | 'OFFLINE',
    }));
  },

  /**
   * Get all slave devices for the current user
   */
  getSlaveDevices: async (): Promise<SlaveDevice[]> => {
    const response = await apiClient.get<SlaveDeviceListResponse>('/devices/getSlaveDevice');

    if (!response.success) {
      throw new Error(response.message || 'Failed to fetch slave devices');
    }

    // Normalize connection_status
    return (response.devices || []).map((device) => ({
      ...device,
      connection_status: device.connection_status ||
        (device.is_active ? 'ONLINE' : 'OFFLINE') as 'ONLINE' | 'OFFLINE',
    }));
  },

  /**
   * Get device by ID
   */
  getDevice: async (deviceId: number): Promise<Device> => {
    const response = await apiClient.get<DeviceInfoResponse>(
      `/devices/getDevice?device_id=${deviceId}`
    );

    if (!response.success || !response.device || response.device.length === 0) {
      throw new Error(response.message || 'Device not found');
    }

    return response.device[0];
  },

  /**
   * Register a new device
   */
  registerDevice: async (deviceData: RegisterDeviceRequest): Promise<void> => {
    const response = await apiClient.post<RegisterDeviceResponse>(
      '/devices/registerDevide',
      deviceData
    );

    if (!response.success) {
      throw new Error(response.message || 'Failed to register device');
    }
  },

  /**
   * Delete a device
   */
  deleteDevice: async (deviceToken: string): Promise<void> => {
    const response = await apiClient.delete<RegisterDeviceResponse>(
      `/devices/deleteDevice?device_token=${deviceToken}`
    );

    if (!response.success) {
      throw new Error(response.message || 'Failed to delete device');
    }
  },

  /**
   * Get configuration pins for a device
   */
  getConfigPins: async (deviceToken: string): Promise<ConfigPin[]> => {
    try {
      const response = await apiClient.get<ConfigPinListResponse>(
        `/devices/getConfigPin?device_token=${deviceToken}`
      );

      if (!response.success) {
        // If config pins not found, return empty array
        if (response.message?.includes('not found') ||
          response.message?.includes('Config pin not found')) {
          return [];
        }
        throw new Error(response.message || 'Failed to fetch config pins');
      }

      return response.config_pins || [];
    } catch (error) {
      // If error is about not found, return empty array
      if (error instanceof Error &&
        (error.message.includes('not found') ||
          error.message.includes('Config pin not found'))) {
        return [];
      }
      throw error;
    }
  },

  /**
   * Save configuration pins for a device
   */
  saveConfigPins: async (configData: ConfigPinRequest): Promise<void> => {
    const response = await apiClient.post<ConfigPinResponse>(
      '/devices/configPin',
      configData
    );

    if (!response.success) {
      throw new Error(response.message || 'Failed to save config pins');
    }
  },

  /**
   * Delete a configuration pin
   */
  deleteConfigPin: async (deviceToken: string, virtualPin: number): Promise<void> => {
    const response = await apiClient.delete<ConfigPinResponse>(
      `/devices/deleteConfigPin?device_token=${deviceToken}&virtual_pin=${virtualPin}`
    );

    if (!response.success) {
      throw new Error(response.message || 'Failed to delete config pin');
    }
  },
};

export default deviceService;

