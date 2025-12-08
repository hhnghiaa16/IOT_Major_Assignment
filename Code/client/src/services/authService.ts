/**
 * Authentication Service
 * Handles all authentication-related API calls
 */

import apiClient from './apiClient';
import type { LoginRequest, RegisterRequest, RegisterViewerRequest, AuthResponse } from '../types';

export const authService = {
  /**
   * Login user
   */
  login: async (credentials: LoginRequest): Promise<AuthResponse> => {
    const response = await apiClient.post<AuthResponse>('/auth/login', credentials);
    
    if (!response.access_token) {
      throw new Error(response.message || 'Login failed');
    }
    
    return response;
  },

  /**
   * Register new user
   */
  register: async (userData: RegisterRequest): Promise<AuthResponse> => {
    const response = await apiClient.post<AuthResponse>('/auth/register', userData);
    
    if (!response.access_token) {
      throw new Error(response.message || 'Registration failed');
    }
    
    return response;
  },

  /**
   * Register new viewer/guest user
   */
  registerViewer: async (userData: RegisterViewerRequest): Promise<AuthResponse> => {
    const response = await apiClient.post<AuthResponse>('/auth/register_viewer', userData);
    
    if (!response.access_token) {
      throw new Error(response.message || 'Registration failed');
    }
    
    return response;
  },

  /**
   * Store authentication data in localStorage
   */
  storeAuthData: (accessToken: string, user?: unknown): void => {
    localStorage.setItem('access_token', accessToken);
    if (user) {
      localStorage.setItem('user', JSON.stringify(user));
    }
  },

  /**
   * Clear authentication data from localStorage
   */
  clearAuthData: (): void => {
    localStorage.removeItem('access_token');
    localStorage.removeItem('user');
  },

  /**
   * Check if user is authenticated
   */
  isAuthenticated: (): boolean => {
    return Boolean(localStorage.getItem('access_token'));
  },
};

export default authService;

