/**
 * useLogin Hook
 * Handles login and registration logic
 */

import { useState } from 'react';
import authService from '../services/authService';
import type { AuthMode, User } from '../types';

interface UseLoginReturn {
  mode: AuthMode;
  setMode: (mode: AuthMode) => void;
  email: string;
  setEmail: (email: string) => void;
  name: string;
  setName: (name: string) => void;
  password: string;
  setPassword: (password: string) => void;
  loading: boolean;
  error: string | null;
  success: string | null;
  resetMessages: () => void;
  handleSubmit: (e: React.FormEvent, onLogin?: (user?: User) => void) => Promise<void>;
}

export const useLogin = (): UseLoginReturn => {
  const [mode, setMode] = useState<AuthMode>('login');
  const [email, setEmail] = useState('');
  const [name, setName] = useState('');
  const [password, setPassword] = useState('');
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [success, setSuccess] = useState<string | null>(null);

  const resetMessages = () => {
    setError(null);
    setSuccess(null);
  };

  const handleSubmit = async (
    e: React.FormEvent,
    onLogin?: (user?: User) => void
  ): Promise<void> => {
    e.preventDefault();
    resetMessages();

    // Validation
    if (!email || !password || (mode === 'register' && !name)) {
      setError('Vui lòng điền đầy đủ thông tin');
      return;
    }

    setLoading(true);
    try {
      let response;
      
      if (mode === 'login') {
        response = await authService.login({ email, password });
      } else {
        response = await authService.register({
          email,
          password,
          name,
          type: 1,
        });
      }

      // Store auth data
      authService.storeAuthData(response.access_token, response.user);

      setSuccess(
        mode === 'login' ? 'Đăng nhập thành công' : 'Tạo tài khoản thành công'
      );
      setPassword('');
      if (mode === 'register') {
        setName('');
      }

      // Call onLogin callback if provided
      if (onLogin && response.user) {
        onLogin(response.user);
      }
    } catch (err) {
      const errorMessage = err instanceof Error ? err.message : 'Lỗi không xác định';
      setError(errorMessage);
    } finally {
      setLoading(false);
    }
  };

  return {
    mode,
    setMode,
    email,
    setEmail,
    name,
    setName,
    password,
    setPassword,
    loading,
    error,
    success,
    resetMessages,
    handleSubmit,
  };
};

export default useLogin;

