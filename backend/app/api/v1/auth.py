from __future__ import annotations

from datetime import UTC, datetime
from uuid import UUID

import jwt
from fastapi import APIRouter, Depends, HTTPException
from sqlalchemy import select
from sqlalchemy.ext.asyncio import AsyncSession

from app.api.dependencies import current_user, get_session, get_settings, require_roles
from app.core.config import Settings
from app.core.security import create_token, decode_token, hash_password, verify_password
from app.models import User
from app.schemas.auth import LoginRequest, RefreshRequest, TokenPair, UserCreate, UserPublic

router = APIRouter(prefix="/auth", tags=["authentication"])


def token_pair(user: User, settings: Settings) -> TokenPair:
    return TokenPair(
        access_token=create_token(user.id, user.role, "access", settings),
        refresh_token=create_token(user.id, user.role, "refresh", settings),
    )


@router.post("/login", response_model=TokenPair)
async def login(
    request: LoginRequest,
    session: AsyncSession = Depends(get_session),
    settings: Settings = Depends(get_settings),
) -> TokenPair:
    user = await session.scalar(select(User).where(User.email == request.email.lower()))
    if user is None or not user.active or not verify_password(request.password, user.password_hash):
        raise HTTPException(status_code=401, detail="invalid credentials")
    user.last_login_at = datetime.now(UTC)
    await session.commit()
    return token_pair(user, settings)


@router.post("/refresh", response_model=TokenPair)
async def refresh(
    request: RefreshRequest,
    session: AsyncSession = Depends(get_session),
    settings: Settings = Depends(get_settings),
) -> TokenPair:
    try:
        claims = decode_token(request.refresh_token, "refresh", settings)
        user = await session.get(User, UUID(claims["sub"]))
    except (jwt.InvalidTokenError, ValueError):
        raise HTTPException(status_code=401, detail="invalid refresh token") from None
    if user is None or not user.active:
        raise HTTPException(status_code=401, detail="inactive user")
    return token_pair(user, settings)


@router.get("/me", response_model=UserPublic)
async def me(user: User = Depends(current_user)) -> User:
    return user


@router.post("/users", response_model=UserPublic, status_code=201)
async def create_user(
    request: UserCreate,
    session: AsyncSession = Depends(get_session),
    _: User = Depends(require_roles("admin")),
) -> User:
    email = request.email.lower()
    if await session.scalar(select(User.id).where(User.email == email)) is not None:
        raise HTTPException(status_code=409, detail="email already registered")
    user = User(email=email, password_hash=hash_password(request.password), role=request.role)
    session.add(user)
    await session.commit()
    await session.refresh(user)
    return user
