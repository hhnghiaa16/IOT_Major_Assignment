/**
 * useAuth Hook
 * Manages authentication state and operations
 */

import { useState, useEffect } from 'react';
import authService from '../services/authService';
import type { User } from '../types';

export type UserRole = 'admin' | 'viewer';

export const useAuth = () => {
  const [isLoggedIn, setIsLoggedIn] = useState<boolean>(false);
  const [user, setUser] = useState<User | null>(null);

  // Load user from localStorage on mount
  useEffect(() => {
    const checkAuth = () => {
      const isAuth = authService.isAuthenticated();
      setIsLoggedIn(isAuth);
      
      if (isAuth) {
        const userStr = localStorage.getItem('user');
        if (userStr) {
          try {
            const parsedUser = JSON.parse(userStr) as User;
            setUser(parsedUser);
          } catch {
            setUser(null);
          }
        }
      } else {
        setUser(null);
      }
    };

    checkAuth();
    
    // Listen for storage changes (e.g., when user logs in from another tab)
    const handleStorageChange = () => {
      checkAuth();
    };
    
    window.addEventListener('storage', handleStorageChange);
    return () => window.removeEventListener('storage', handleStorageChange);
  }, []);

  const logout = () => {
    authService.clearAuthData();
    setIsLoggedIn(false);
    setUser(null);
  };

  const login = (userData?: User) => {
    setIsLoggedIn(true);
    if (userData) {
      setUser(userData);
    } else {
      // Load user from localStorage if not provided
      const userStr = localStorage.getItem('user');
      if (userStr) {
        try {
          const parsedUser = JSON.parse(userStr) as User;
          setUser(parsedUser);
        } catch {
          setUser(null);
        }
      }
    }
  };

  /**
   * Get user role
   * @returns 'admin' | 'viewer' | null
   */
  const getUserRole = (): UserRole | null => {
    if (!user || user.type === undefined) return null;
    return user.type === 0 ? 'admin' : 'viewer';
  };

  /**
   * Check if user has permission for a specific role
   * @param role - The role to check ('admin' | 'viewer')
   * @returns boolean
   */
  const hasPermission = (role: UserRole): boolean => {
    const userRole = getUserRole();
    if (!userRole) return false;
    
    // Admin has all permissions
    if (userRole === 'admin') return true;
    
    // Viewer only has viewer permissions
    return role === 'viewer';
  };

  /**
   * Check if user is admin
   */
  const isAdmin = (): boolean => {
    return getUserRole() === 'admin';
  };

  /**
   * Check if user is viewer
   */
  const isViewer = (): boolean => {
    return getUserRole() === 'viewer';
  };

  return {
    isLoggedIn,
    user,
    login,
    logout,
    getUserRole,
    hasPermission,
    isAdmin,
    isViewer,
  };
};

export default useAuth;

