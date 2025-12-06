/**
 * API Client - Centralized API configuration and utilities
 */

const API_BASE = process.env.REACT_APP_API_URL || 'http://localhost:8000';

/**
 * Get authentication token from localStorage
 */
export const getAuthToken = (): string | null => {
  return localStorage.getItem('access_token');
};

/**
 * Get default headers with authentication
 */
export const getAuthHeaders = (): HeadersInit => {
  const token = getAuthToken();
  return {
    'Content-Type': 'application/json',
    ...(token && { Authorization: `Bearer ${token}` }),
  };
};

/**
 * Handle API response and parse JSON
 */
export const handleApiResponse = async <T>(response: Response): Promise<T> => {
  const data = await response.json();
  
  if (!response.ok) {
    const errorMessage = data.detail || data.message || 'API request failed';
    throw new Error(typeof errorMessage === 'string' ? errorMessage : 'API request failed');
  }
  
  return data;
};

/**
 * Base API client function
 */
export const apiClient = {
  get: async <T>(endpoint: string): Promise<T> => {
    const response = await fetch(`${API_BASE}${endpoint}`, {
      method: 'GET',
      headers: getAuthHeaders(),
    });
    return handleApiResponse<T>(response);
  },

  post: async <T>(endpoint: string, body?: unknown): Promise<T> => {
    const response = await fetch(`${API_BASE}${endpoint}`, {
      method: 'POST',
      headers: getAuthHeaders(),
      body: body ? JSON.stringify(body) : undefined,
    });
    return handleApiResponse<T>(response);
  },

  put: async <T>(endpoint: string, body?: unknown): Promise<T> => {
    const response = await fetch(`${API_BASE}${endpoint}`, {
      method: 'PUT',
      headers: getAuthHeaders(),
      body: body ? JSON.stringify(body) : undefined,
    });
    return handleApiResponse<T>(response);
  },

  delete: async <T>(endpoint: string): Promise<T> => {
    const response = await fetch(`${API_BASE}${endpoint}`, {
      method: 'DELETE',
      headers: getAuthHeaders(),
    });
    return handleApiResponse<T>(response);
  },
};

export default apiClient;

