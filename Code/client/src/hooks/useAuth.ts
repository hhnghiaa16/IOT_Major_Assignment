/**
 * useAuth Hook
 * Manages authentication state and operations
 */

import { useState, useEffect } from 'react';
import authService from '../services/authService';

export const useAuth = () => {
  const [isLoggedIn, setIsLoggedIn] = useState<boolean>(false);

  useEffect(() => {
    setIsLoggedIn(authService.isAuthenticated());
  }, []);

  const logout = () => {
    authService.clearAuthData();
    setIsLoggedIn(false);
  };

  const login = () => {
    setIsLoggedIn(true);
  };

  return {
    isLoggedIn,
    login,
    logout,
  };
};

export default useAuth;

