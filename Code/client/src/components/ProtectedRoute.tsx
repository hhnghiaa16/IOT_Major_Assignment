/**
 * ProtectedRoute Component
 * Route protection with role-based access control
 */

import React from 'react';
import { useAuth } from '../hooks/useAuth';
import type { UserRole } from '../hooks/useAuth';

interface ProtectedRouteProps {
  children: React.ReactNode;
  allowedRoles?: UserRole[];
  redirectTo?: string;
}

const ProtectedRoute: React.FC<ProtectedRouteProps> = ({
  children,
  allowedRoles = ['admin', 'viewer'],
}) => {
  const { isLoggedIn, user, getUserRole } = useAuth();

  // Don't render if not authenticated
  if (!isLoggedIn) {
    return null;
  }

  // Wait for user to be loaded before checking role
  if (!user) {
    return null;
  }

  const userRole = getUserRole();

  // If no role or role not allowed, don't render
  if (!userRole || !allowedRoles.includes(userRole)) {
    return null;
  }

  return <>{children}</>;
};

export default ProtectedRoute;

