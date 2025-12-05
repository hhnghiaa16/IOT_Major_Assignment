/**
 * Formatter Utilities
 * Pure functions for formatting data
 */

/**
 * Format timestamp to time string (HH:mm)
 */
export const formatTimestamp = (timestamp: string): string => {
  try {
    const date = new Date(timestamp);
    return date.toLocaleTimeString('en-US', {
      hour: '2-digit',
      minute: '2-digit',
      hour12: false,
    });
  } catch {
    return timestamp;
  }
};

/**
 * Format timestamp to date string (MMM dd)
 */
export const formatDate = (timestamp: string): string => {
  try {
    const date = new Date(timestamp);
    return date.toLocaleDateString('en-US', {
      month: 'short',
      day: 'numeric',
    });
  } catch {
    return '';
  }
};

/**
 * Format value to fixed decimal places
 */
export const formatValue = (val: number, decimals: number = 1): string => {
  return val.toFixed(decimals);
};

/**
 * Format date to Vietnamese locale string
 */
export const formatDateTime = (dateString: string): string => {
  try {
    return new Date(dateString).toLocaleString('vi-VN');
  } catch {
    return 'N/A';
  }
};

/**
 * Get data type label
 */
export const getDataTypeLabel = (type: string): string => {
  const labels: Record<string, string> = {
    string: 'String',
    float: 'Float',
    integer: 'Integer',
    boolean: 'Boolean',
    datetime: 'Datetime',
    json: 'JSON',
  };
  return labels[type] || type;
};

/**
 * Get pin type label
 */
export const getPinTypeLabel = (type: string): string => {
  if (type === 'INPUT') return 'INPUT (Từ thiết bị lên)';
  if (type === 'OUTPUT') return 'OUTPUT (Từ server xuống)';
  return type;
};

