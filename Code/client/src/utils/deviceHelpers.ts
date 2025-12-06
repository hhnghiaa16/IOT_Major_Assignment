/**
 * Device Helper Utilities
 * Pure functions for device-related operations
 */

import type { Device } from '../types';

/**
 * Get status text from device
 */
export const getStatusText = (device: Device): string => {
  if (device.connection_status) {
    return device.connection_status.toUpperCase();
  }
  const isOnline = device.is_online ?? device.is_active ?? false;
  return isOnline ? 'ONLINE' : 'OFFLINE';
};

/**
 * Get status CSS class from device
 */
export const getStatusClass = (device: Device): string => {
  if (device.connection_status) {
    const status = device.connection_status.toUpperCase();
    return status === 'ONLINE' ? 'device-status-online' : 'device-status-offline';
  }
  const isOnline = device.is_online ?? device.is_active ?? false;
  return isOnline ? 'device-status-online' : 'device-status-offline';
};

/**
 * Get device type CSS class
 */
export const getTypeClass = (type: string): string => {
  return type === 'MASTER' ? 'device-type-master' : 'device-type-slave';
};

/**
 * Copy text to clipboard
 */
export const copyToClipboard = async (text: string): Promise<boolean> => {
  try {
    await navigator.clipboard.writeText(text);
    return true;
  } catch {
    return false;
  }
};

